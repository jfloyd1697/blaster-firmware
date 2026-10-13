#ifdef PLATFORM_STM32F411

#include "Stm32Bridge.h"

#include <algorithm>
#include <cstring>

#include "core/audio/IAudioEngine.h"
#include "core/display/IStatusDisplay.h"
#include "core/input/IInput.h"
#include "core/lights/ILights.h"
#include "core/lights/LightEngine.h"
#include "core/text_resource_loader/ITextResourceLoader.h"
#include "platform/stm32f411/storage/Stm32F411Sd.h"

namespace {
constexpr std::size_t kSerialBudget = 64;
constexpr std::size_t kReadChunk = 128;
constexpr std::size_t kListPageSize = 256;
constexpr std::size_t kHashChunk = 512;
constexpr std::size_t kMaximumPath = 180;
constexpr std::uint32_t kSessionTimeoutMs = 3000;

bool hasSuffix(const std::string& value, const char* suffix) {
    const auto size = std::strlen(suffix);
    return value.size() >= size && value.compare(value.size() - size, size, suffix) == 0;
}
}

Stm32Bridge::Stm32Bridge(PlatformServices& services) : m_services(services) {}

void Stm32Bridge::reply(bridge::Op op, std::uint16_t sequence, const std::uint8_t* payload, std::size_t size) {
    if (!Serial || size > bridge::kMaximumPayload) return;
    std::uint8_t header[7]{bridge::kSyncFirst, bridge::kSyncSecond};
    bridge::write16(header + 2, static_cast<std::uint16_t>(size));
    header[4] = static_cast<std::uint8_t>(op);
    bridge::write16(header + 5, sequence);
    std::uint16_t crc = bridge::crc16(header + 2, 5);
    // Incremental CRC: the protocol CRC covers header and payload without sync.
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= static_cast<std::uint16_t>(payload[i]) << 8;
        for (int bit = 0; bit < 8; ++bit)
            crc = static_cast<std::uint16_t>((crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0));
    }
    std::uint8_t footer[2];
    bridge::write16(footer, crc);
    Serial.write(header, sizeof(header));
    if (size) Serial.write(payload, size);
    Serial.write(footer, sizeof(footer));
}

void Stm32Bridge::acknowledge(std::uint16_t seq, bridge::Result status,
                             const std::uint8_t* extra, std::size_t length) {
    std::uint8_t payload[bridge::kMaximumPayload]{};
    payload[0] = static_cast<std::uint8_t>(status);
    if (length && extra) std::memcpy(payload + 1, extra, length);
    reply(bridge::Op::Ack, seq, payload, length + 1);
}

void Stm32Bridge::setMode(Mode mode) {
    if (m_services.audio) m_services.audio->stop();
    if (m_services.lights) m_services.lights->stopSequence();
    if (m_services.input) m_services.input->setRemoteInputEnabled(mode == Mode::RemoteInput);
    m_mode = mode;
    if (m_services.display) {
        const auto displayMode = mode == Mode::RemoteInput ? DisplayControlMode::RemoteInput
            : mode == Mode::DirectControl ? DisplayControlMode::DirectControl : DisplayControlMode::Normal;
        m_services.display->setControlMode(displayMode);
    }
    m_lastHeartbeatMs = millis();
}

void Stm32Bridge::enterMaintenance() {
    if (m_maintenance) return;
    if (m_services.audio) {
        m_services.audio->stop();
        m_services.audio->releasePreparedSounds();
    }
    if (m_services.lights) m_services.lights->stopSequence();
    m_maintenance = true;
    m_lastHeartbeatMs = millis();
}

void Stm32Bridge::abortUpload() {
    if (m_upload.isOpen()) m_upload.close();
    if (!m_stagePath.empty()) g_stm32Sd.remove(m_stagePath.c_str());
    m_targetPath.clear();
    m_stagePath.clear();
    m_expectedSize = 0;
    m_receivedSize = 0;
}

void Stm32Bridge::leaveMaintenance() {
    abortUpload();
    if (m_hashFile.isOpen()) m_hashFile.close();
    m_hashPending = false;
    if (m_maintenance) m_activate = true;
    m_maintenance = false;
}

bool Stm32Bridge::takeActivate() {
    if (!m_activate) return false;
    m_activate = false;
    return true;
}

void Stm32Bridge::finishedActivation() {
    m_maintenance = false;
}

bool Stm32Bridge::validPath(const std::string& path, bool rootAllowed) const {
    if (path.empty()) return rootAllowed;
    if (path.size() > kMaximumPath || path.front() == '/' || path.back() == '/') return false;
    std::size_t segmentStart = 0;
    for (std::size_t i = 0; i <= path.size(); ++i) {
        if (i != path.size() && path[i] != '/') {
            const unsigned char ch = static_cast<unsigned char>(path[i]);
            if (ch < 33 || ch > 126 || std::strchr("\\:*?\"<>|", ch)) return false;
            continue;
        }
        const auto segment = path.substr(segmentStart, i - segmentStart);
        if (segment.empty() || segment == "." || segment == ".." ||
            hasSuffix(segment, ".upload") || hasSuffix(segment, ".backup")) return false;
        segmentStart = i + 1;
    }
    return true;
}

bool Stm32Bridge::decodePath(const std::uint8_t* data, std::size_t size,
                             std::string& path, bool rootAllowed) const {
    path.assign(reinterpret_cast<const char*>(data), size);
    return validPath(path, rootAllowed);
}

void Stm32Bridge::serviceHash() {
    if (!m_hashPending) return;
    std::uint8_t data[kHashChunk];
    const int count = m_hashFile.read(data, sizeof(data));
    if (count < 0) {
        m_hashFile.close();
        m_hashPending = false;
        acknowledge(m_hashSequence, bridge::Result::IoError);
        return;
    }
    if (count > 0) m_hashCrc = bridge::crc32Update(m_hashCrc, data, static_cast<std::size_t>(count));
    if (m_hashFile.available() && count > 0) return;
    m_hashFile.close();
    m_hashPending = false;
    std::uint8_t result[8];
    bridge::write32(result, m_hashSize);
    bridge::write32(result + 4, ~m_hashCrc);
    acknowledge(m_hashSequence, bridge::Result::Ok, result, sizeof(result));
}

bridge::Result Stm32Bridge::fileCommand(const bridge::Frame& frame,
                                       std::uint8_t* result, std::size_t& resultSize) {
    using bridge::Op;
    using bridge::Result;
    const auto* data = frame.data.data();
    const std::size_t size = frame.size;
    const Op op = frame.op;
    std::string path;
    if (m_hashPending) return Result::Busy;
    if (op == Op::FileAbort) {
        abortUpload();
        return Result::Ok;
    }
    if (op == Op::FileBegin) {
        if (size < 5 || m_upload.isOpen() ||
            !decodePath(data + 4, size - 4, path)) return Result::Invalid;
        m_targetPath = path;
        m_stagePath = path + ".upload";
        // A crashed or unplugged transfer can leave an abandoned staging file.
        // The single MCU-owned filesystem permits safe cleanup before a new upload.
        if (g_stm32Sd.exists(m_stagePath.c_str()) && !g_stm32Sd.remove(m_stagePath.c_str())) {
            return Result::IoError;
        }
        const std::string backup = path + ".backup";
        if (g_stm32Sd.exists(backup.c_str())) {
            if (g_stm32Sd.exists(path.c_str())) {
                if (!g_stm32Sd.remove(backup.c_str())) return Result::IoError;
            } else if (!g_stm32Sd.rename(backup.c_str(), path.c_str())) {
                return Result::IoError;
            }
        }
        const auto parentEnd = path.find_last_of('/');
        if (parentEnd != std::string::npos) {
            const auto parent = path.substr(0, parentEnd);
            if (!g_stm32Sd.mkdir(parent.c_str(), true)) return Result::IoError;
        }
        m_upload = g_stm32Sd.open(m_stagePath.c_str(), O_WRONLY | O_CREAT | O_EXCL);
        if (!m_upload) return Result::IoError;
        m_expectedSize = bridge::read32(data);
        m_receivedSize = 0;
        m_uploadCrc = 0xFFFFFFFFu;
        return Result::Ok;
    }
    if (op == Op::FileChunk) {
        if (!m_upload.isOpen() || size > m_expectedSize - m_receivedSize) return Result::Invalid;
        if (m_upload.write(data, size) != size) {
            abortUpload();
            return Result::IoError;
        }
        m_receivedSize += static_cast<std::uint32_t>(size);
        m_uploadCrc = bridge::crc32Update(m_uploadCrc, data, size);
        return Result::Ok;
    }
    if (op == Op::FileCommit) {
        if (size != 4 || !m_upload.isOpen()) return Result::Invalid;
        const bool verified = m_receivedSize == m_expectedSize && ~m_uploadCrc == bridge::read32(data);
        const bool flushed = m_upload.sync();
        m_upload.close();
        if (!verified || !flushed) {
            abortUpload();
            return verified ? Result::IoError : Result::ChecksumMismatch;
        }
        const std::string backup = m_targetPath + ".backup";
        if (g_stm32Sd.exists(backup.c_str())) return Result::Busy;
        const bool hadOriginal = g_stm32Sd.exists(m_targetPath.c_str());
        if (hadOriginal && !g_stm32Sd.rename(m_targetPath.c_str(), backup.c_str())) {
            abortUpload();
            return Result::IoError;
        }
        if (!g_stm32Sd.rename(m_stagePath.c_str(), m_targetPath.c_str())) {
            if (hadOriginal) g_stm32Sd.rename(backup.c_str(), m_targetPath.c_str());
            abortUpload();
            return Result::IoError;
        }
        if (hadOriginal) g_stm32Sd.remove(backup.c_str());
        m_stagePath.clear();
        m_targetPath.clear();
        return Result::Ok;
    }
    if (m_upload.isOpen()) return Result::Busy;
    if (op == Op::FileList) {
        if (size < 2 || !decodePath(data + 2, size - 2, path, true)) return Result::Invalid;
        const auto offset = bridge::read16(data);
        File32 directory = g_stm32Sd.open(path.empty() ? "/" : path.c_str(), O_RDONLY);
        if (!directory || !directory.isDir()) return Result::NotFound;
        File32 entry;
        std::uint16_t index = 0;
        result[0] = 0; // more pages
        resultSize = 3;
        while (entry.openNext(&directory, O_RDONLY)) {
            if (index++ < offset) { entry.close(); continue; }
            char name[256]{};
            entry.getName(name, sizeof(name));
            const std::size_t length = std::strlen(name);
            if (length > 0 && length <= 255 && resultSize + 6 + length <= kListPageSize) {
                result[resultSize++] = entry.isDir() ? 2 : 1;
                bridge::write32(result + resultSize, entry.isDir() ? 0 : entry.fileSize());
                resultSize += 4;
                result[resultSize++] = static_cast<std::uint8_t>(length);
                std::memcpy(result + resultSize, name, length);
                resultSize += length;
            } else {
                result[0] = 1;
                entry.close();
                --index;
                break;
            }
            entry.close();
        }
        directory.close();
        if (index == offset && result[0]) return Result::Invalid; // name cannot fit page
        bridge::write16(result + 1, index);
        return Result::Ok;
    }
    if (op == Op::FileStat) {
        if (!decodePath(data, size, path)) return Result::Invalid;
        File32 file = g_stm32Sd.open(path.c_str(), O_RDONLY);
        result[0] = !file ? 0 : file.isDir() ? 2 : 1;
        bridge::write32(result + 1, !file || file.isDir() ? 0 : file.fileSize());
        resultSize = 5;
        if (file) file.close();
        return Result::Ok;
    }
    if (op == Op::FileRead) {
        if (size < 7 || !decodePath(data + 6, size - 6, path)) return Result::Invalid;
        const auto count = bridge::read16(data + 4);
        if (count > kReadChunk) return Result::Invalid;
        File32 file = g_stm32Sd.open(path.c_str(), O_RDONLY);
        if (!file || file.isDir()) return Result::NotFound;
        if (!file.seekSet(bridge::read32(data))) { file.close(); return Result::Invalid; }
        const int read = file.read(result, count);
        file.close();
        if (read < 0) return Result::IoError;
        resultSize = static_cast<std::size_t>(read);
        return Result::Ok;
    }
    if (op == Op::FileHash) {
        if (!decodePath(data, size, path)) return Result::Invalid;
        m_hashFile = g_stm32Sd.open(path.c_str(), O_RDONLY);
        if (!m_hashFile || m_hashFile.isDir()) return Result::NotFound;
        m_hashSize = m_hashFile.fileSize();
        m_hashCrc = 0xFFFFFFFFu;
        m_hashSequence = frame.sequence;
        m_hashPending = true;
        return Result::Ok;
    }
    if (op == Op::FileDelete) {
        if (!decodePath(data, size, path)) return Result::Invalid;
        File32 file = g_stm32Sd.open(path.c_str(), O_RDONLY);
        if (!file) return Result::NotFound;
        const bool directory = file.isDir();
        file.close();
        const bool ok = directory ? g_stm32Sd.rmdir(path.c_str()) : g_stm32Sd.remove(path.c_str());
        return ok ? Result::Ok : Result::IoError;
    }
    if (op == Op::FileMkdir) {
        if (!decodePath(data, size, path)) return Result::Invalid;
        return g_stm32Sd.mkdir(path.c_str(), true) ? Result::Ok : Result::IoError;
    }
    if (op == Op::FileRename) {
        const auto* split = static_cast<const std::uint8_t*>(std::memchr(data, 0, size));
        if (!split || !decodePath(data, split - data, path)) return Result::Invalid;
        std::string destination;
        if (!decodePath(split + 1, size - (split - data) - 1, destination)) return Result::Invalid;
        if (g_stm32Sd.exists(destination.c_str())) return Result::Busy;
        return g_stm32Sd.rename(path.c_str(), destination.c_str()) ? Result::Ok : Result::IoError;
    }
    return Result::Invalid;
}

void Stm32Bridge::command(const bridge::Frame& frame) {
    using bridge::Op;
    using bridge::Result;
    const auto op = frame.op;
    const auto* data = frame.data.data();
    const auto length = frame.size;
    m_lastHeartbeatMs = millis();
    if (op == Op::Hello) {
        const std::uint8_t hello[] = {2, 'S', 'T', 'M', '3', '2', 'F', '4', '1', '1', ' ', 'S', 'D'};
        reply(Op::Status, frame.sequence, hello, sizeof(hello));
        return;
    }
    if (op == Op::Heartbeat) { acknowledge(frame.sequence, Result::Ok); return; }
    if (op == Op::SetMode) {
        if (length != 1 || data[0] > 2 || m_maintenance) {
            acknowledge(frame.sequence, Result::InvalidMode);
            return;
        }
        setMode(static_cast<Mode>(data[0]));
        acknowledge(frame.sequence, Result::Ok);
        return;
    }
    if (op == Op::Activate || op == Op::FileRelease) {
        if (m_upload.isOpen() || m_hashPending) {
            acknowledge(frame.sequence, Result::Busy);
            return;
        }
        leaveMaintenance();
        if (op == Op::Activate) m_activate = true;
        acknowledge(frame.sequence, Result::Ok);
        return;
    }
    if (op >= Op::FileBegin && op <= Op::FileHash && op != Op::Status) {
        if (m_mode != Mode::Local) { acknowledge(frame.sequence, Result::InvalidMode); return; }
        enterMaintenance();
        std::uint8_t payload[bridge::kMaximumPayload]{};
        std::size_t payloadSize = 0;
        const Result status = fileCommand(frame, payload, payloadSize);
        if (op != Op::FileHash || status != Result::Ok)
            acknowledge(frame.sequence, status, payload, payloadSize);
        return;
    }
    if (m_maintenance) { acknowledge(frame.sequence, Result::Busy); return; }
    if (op == Op::Input && m_mode == Mode::RemoteInput && length == 2 && data[0] <= 3 && data[1] <= 1) {
        const ButtonID ids[] = {ButtonID::Trigger, ButtonID::NextWeapon, ButtonID::PreviousWeapon, ButtonID::Reload};
        if (m_services.input) m_services.input->setRemoteButton(ids[data[0]], data[1] != 0);
        acknowledge(frame.sequence, Result::Ok);
    } else if (op == Op::StopAudio && m_mode == Mode::DirectControl) {
        if (m_services.audio) m_services.audio->stop();
        acknowledge(frame.sequence, Result::Ok);
    } else if (op == Op::StopLight && m_mode == Mode::DirectControl) {
        if (m_services.lights) m_services.lights->stopSequence();
        acknowledge(frame.sequence, Result::Ok);
    } else if (op == Op::PlayAudio && m_mode == Mode::DirectControl && length > 1) {
        if (m_services.audio) m_services.audio->playSound(std::string(reinterpret_cast<const char*>(data + 1), length - 1), data[0] != 0, false);
        acknowledge(frame.sequence, Result::Ok);
    } else if (op == Op::PlayLight && m_mode == Mode::DirectControl && length > 1) {
        LightSequencePlayback playback;
        playback.loop = data[0] != 0;
        const bool ok = m_services.lights && m_services.textLoader &&
            m_services.lights->playSequenceFile(std::string(reinterpret_cast<const char*>(data + 1), length - 1),
                *m_services.textLoader, playback);
        acknowledge(frame.sequence, ok ? Result::Ok : Result::IoError);
    } else if (op == Op::LedFrame && m_mode == Mode::DirectControl && length >= 5 &&
               bridge::read16(data) * 3u == length - 2u) {
        auto* lights = static_cast<LightEngine*>(m_services.lights.get());
        if (!lights) { acknowledge(frame.sequence, Result::IoError); return; }
        std::vector<LightColor> pixels;
        const auto count = bridge::read16(data);
        pixels.reserve(count);
        for (std::size_t index = 0; index < count; ++index) {
            pixels.push_back({data[2 + index * 3], data[3 + index * 3], data[4 + index * 3]});
        }
        lights->displayDirectFrame(std::move(pixels));
        acknowledge(frame.sequence, Result::Ok);
    } else {
        acknowledge(frame.sequence, Result::InvalidMode);
    }
}

void Stm32Bridge::service() {
    if (!Serial) {
        if (m_maintenance || m_mode != Mode::Local) {
            setMode(Mode::Local);
            leaveMaintenance();
        }
        return;
    }
    bridge::Frame frame;
    std::size_t consumed = 0;
    while (Serial.available() && consumed++ < kSerialBudget) {
        if (m_parser.push(static_cast<std::uint8_t>(Serial.read()), frame)) {
            command(frame);
            break; // one SD operation per foreground pass
        }
    }
    if (m_hashPending) serviceHash();
    if ((m_maintenance || m_mode != Mode::Local) && millis() - m_lastHeartbeatMs > kSessionTimeoutMs) {
        setMode(Mode::Local);
        leaveMaintenance();
    }
}

#endif
