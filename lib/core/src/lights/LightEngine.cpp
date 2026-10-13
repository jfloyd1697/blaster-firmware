#include "core/lights/LightEngine.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "core/time/ITime.h"
#include "core/text_resource_loader/ITextResourceLoader.h"
#include "weapon_behavior/WeaponBehaviorTypes.h"

namespace {
constexpr int kMaximumChannel = 255;
constexpr int kDefaultBrightness = 255;
constexpr int kDefaultFlashDurationMs = 60;
constexpr int kDefaultFlashCount = 1;
constexpr int kDefaultFlashGapMs = 40;

int clampByte(const int value) {
    return std::clamp(value, 0, kMaximumChannel);
}
}

LightEngine::LightEngine(std::unique_ptr<ILightBackend> backend, ITime* time)
    : m_backend(std::move(backend)),
      m_time(time) {
}

bool LightEngine::begin() {
    if (m_backend == nullptr || m_time == nullptr || !m_backend->begin()) {
        return false;
    }
    clear();
    return true;
}

void LightEngine::setPattern(const weapon_behavior::LightPatternDef& pattern) {
    m_basePattern = ActivePattern{pattern, nowMs()};
}

void LightEngine::flashPattern(const weapon_behavior::LightPatternDef& pattern) {
    m_overridePattern = ActivePattern{pattern, nowMs()};
}

void LightEngine::flash() {
    weapon_behavior::LightPatternDef pattern;
    pattern.mode = weapon_behavior::LightPatternMode::Flash;
    pattern.color = {255, 255, 255};
    pattern.brightness = kDefaultBrightness;
    pattern.durationMs = kDefaultFlashDurationMs;
    pattern.count = kDefaultFlashCount;
    pattern.intervalMs = kDefaultFlashGapMs;
    flashPattern(pattern);
}

void LightEngine::playSequence(const LightSequence& sequence, const bool loop) {
    m_pendingSequence.reset();
    m_stream.reset();
    m_sequence = ActiveSequence{sequence, nowMs(), loop};
}

bool LightEngine::playSequenceFile(
    const std::string& path, ITextResourceLoader& loader, const LightSequencePlayback playback) {
    requestSequenceFile(path, loader, playback);
    // Explicit synchronous calls (diagnostics/tests) run the pending foreground steps.
    // Normal weapon transitions use requestSequenceFile, never this synchronous path.
    while (m_pendingSequence) prefetchNextFrame();
    return m_stream.has_value();
}

bool LightEngine::requestSequenceFile(
    const std::string& path, ITextResourceLoader& loader, const LightSequencePlayback playback) {
    m_pendingSequence = PendingSequence{path, &loader, playback};
    m_stream.reset();
    m_sequence.reset();
    return true;
}

void LightEngine::prefetchNextFrame() {
    if (m_pendingSequence) {
        auto& pending = *m_pendingSequence;
        if (!pending.header) {
            LightSequenceHeader header;
            if (!LightSequenceParser::readHeader(*pending.loader, pending.path, header) ||
                pending.playback.firstFrame >= header.frameCount) {
                m_pendingSequence.reset();
                return;
            }
            pending.header = header;
            return;
        }

        // Skip at most one frame per foreground service call, reading only its
        // six-byte metadata; only decode the requested frame's pixels.
        if (pending.nextFrameIndex < pending.playback.firstFrame) {
            std::size_t nextOffset = 0;
            if (!LightSequenceParser::skipFrame(*pending.loader, pending.path, pending.nextOffset,
                                               pending.header->ledCount, nextOffset)) {
                m_pendingSequence.reset();
                return;
            }
            pending.nextOffset = nextOffset;
            ++pending.nextFrameIndex;
            return;
        }
        std::size_t nextOffset = 0;
        LightFrame frame;
        if (!LightSequenceParser::readFrame(*pending.loader, pending.path, pending.nextOffset,
                                            pending.header->ledCount, frame, nextOffset)) {
            m_pendingSequence.reset();
            return;
        }

        StreamingSequence stream;
        stream.loader = pending.loader;
        stream.path = pending.path;
        stream.header = *pending.header;
        stream.frame = std::move(frame);
        stream.firstFrame = stream.frame;
        stream.nextOffset = nextOffset;
        stream.firstNextOffset = nextOffset;
        stream.frameEndMs = stream.frame.durationMs;
        stream.frameIndex = pending.playback.firstFrame;
        stream.firstFrameIndex = pending.playback.firstFrame;
        stream.loop = pending.playback.loop;
        stream.holdFrame = pending.playback.holdFrame;
        stream.startMs = nowMs();
        m_stream = std::move(stream);
        m_pendingSequence.reset();
        return;
    }
    if (!m_stream) return;
    auto& stream = *m_stream;
    if (stream.holdFrame || stream.prefetched || stream.frameIndex + 1 >= stream.header.frameCount) return;
    // One bounded SD read, separated from the rendering/control path.
    std::size_t offset = 0;
    LightFrame next;
    if (!LightSequenceParser::readFrame(*stream.loader, stream.path, stream.nextOffset,
                                        stream.header.ledCount, next, offset)) return;
    stream.prefetchedFrame = std::move(next);
    stream.prefetchedOffset = offset;
    stream.prefetched = true;
}

void LightEngine::stopSequence() {
    m_pendingSequence.reset();
    m_stream.reset();
    m_sequence.reset();
    m_basePattern.reset();
    m_overridePattern.reset();
    clear();
}

void LightEngine::update() {
    const std::uint32_t now = nowMs();

    if (m_overridePattern.has_value()) {
        const auto elapsed = now - m_overridePattern->startMs;
        const PatternState state = evaluatePattern(m_overridePattern->pattern, elapsed, false);
        if (!state.finished) {
            // The override can replace the current streamed frame.
            // Repaint that frame once when the override finishes.
            if (m_stream) m_stream->frameNeedsRender = true;
            renderPatternState(state);
            return;
        }
        m_overridePattern.reset();
    }

    if (m_stream.has_value()) {
        auto& stream = *m_stream;
        const std::uint32_t elapsed = now - stream.startMs;
        if (!stream.holdFrame && elapsed >= stream.frameEndMs) {
            if (stream.frameIndex + 1 == stream.header.frameCount) {
                if (stream.loop) {
                    // Loop the selected segment, not frames preceding firstFrame.
                    stream.frame = stream.firstFrame;
                    stream.nextOffset = stream.firstNextOffset;
                    stream.frameIndex = stream.firstFrameIndex;
                    stream.frameEndMs = stream.frame.durationMs;
                    stream.startMs = now;
                    stream.prefetched = false;
                    stream.frameNeedsRender = true;
                } else {
                    m_stream.reset();
                }
            } else if (stream.prefetched) {
                ++stream.frameIndex;
                stream.nextOffset = stream.prefetchedOffset;
                stream.frameEndMs += stream.prefetchedFrame.durationMs;
                stream.frame = std::move(stream.prefetchedFrame);
                stream.prefetched = false;
                stream.frameNeedsRender = true;
            } else {
                ++m_missedPrefetches;
            }
        }
        if (m_stream) {
            if (stream.frameNeedsRender) {
                renderSequenceFrame(stream.frame);
                stream.frameNeedsRender = false;
            }
            return;
        }
    }

    if (m_sequence.has_value() && !m_sequence->sequence.frames.empty()) {
        std::uint32_t totalMs = 0;
        for (const auto& frame : m_sequence->sequence.frames) {
            totalMs += frame.durationMs;
        }

        const auto elapsed = now - m_sequence->startMs;
        if (totalMs > 0 && (m_sequence->loop || elapsed < totalMs)) {
            const std::uint32_t local = m_sequence->loop ? elapsed % totalMs : elapsed;
            std::uint32_t cursor = 0;
            for (const auto& frame : m_sequence->sequence.frames) {
                cursor += frame.durationMs;
                if (local < cursor) {
                    renderSequenceFrame(frame);
                    return;
                }
            }
        } else {
            m_sequence.reset();
        }
    }

    if (m_basePattern.has_value()) {
        const auto elapsed = now - m_basePattern->startMs;
        renderPatternState(evaluatePattern(m_basePattern->pattern, elapsed, true));
        return;
    }

    clear();
}

std::uint32_t LightEngine::nowMs() const {
    return static_cast<std::uint32_t>(m_time->millis());
}

LightEngine::PatternState LightEngine::evaluatePattern(
    const weapon_behavior::LightPatternDef& pattern,
    const std::uint32_t elapsedMs,
    const bool loop) {
    PatternState state;
    state.color = pattern.color;
    state.brightness = clampByte(pattern.brightness.value_or(kDefaultBrightness));

    switch (pattern.mode) {
        case weapon_behavior::LightPatternMode::Solid:
            if (!loop && elapsedMs >= static_cast<std::uint32_t>(
                    std::max(pattern.durationMs.value_or(kDefaultFlashDurationMs), 1))) {
                state.brightness = 0;
                state.finished = true;
            }
            return state;

        case weapon_behavior::LightPatternMode::Flash: {
            const auto onMs = static_cast<std::uint32_t>(std::max(pattern.durationMs.value_or(0), 1));
            const auto offMs = static_cast<std::uint32_t>(std::max(pattern.intervalMs.value_or(0), 0));
            const auto count = static_cast<std::uint32_t>(std::max(pattern.count.value_or(1), 1));
            const auto cycleMs = onMs + offMs;
            const auto totalMs = cycleMs * count;

            if (!loop && elapsedMs >= totalMs) {
                state.brightness = 0;
                state.finished = true;
                return state;
            }

            if (cycleMs > 0 && elapsedMs % cycleMs >= onMs) {
                state.brightness = 0;
            }
            return state;
        }

        case weapon_behavior::LightPatternMode::Pulse: {
            const auto intervalMs = static_cast<std::uint32_t>(std::max(pattern.intervalMs.value_or(1), 1));
            const auto durationMs = static_cast<std::uint32_t>(
                std::max(pattern.durationMs.value_or(static_cast<int>(intervalMs)), 1));
            if (!loop && elapsedMs >= durationMs) {
                state.brightness = 0;
                state.finished = true;
                return state;
            }

            const auto phase = elapsedMs % intervalMs;
            const auto half = std::max<std::uint32_t>(intervalMs / 2u, 1u);
            const auto ramp = phase <= half ? phase : intervalMs - phase;
            state.brightness = state.brightness * static_cast<int>(ramp) / static_cast<int>(half);
            return state;
        }

        case weapon_behavior::LightPatternMode::Sequence: {
            if (pattern.steps.empty()) {
                state.brightness = 0;
                state.finished = true;
                return state;
            }

            std::uint32_t totalMs = 0;
            for (const auto& step : pattern.steps) {
                totalMs += static_cast<std::uint32_t>(std::max(step.durationMs, 0));
            }

            if (totalMs == 0) {
                state.color = pattern.steps.back().color;
                return state;
            }

            if (!loop && elapsedMs >= totalMs) {
                state.brightness = 0;
                state.finished = true;
                return state;
            }

            const auto sequenceTime = loop ? elapsedMs % totalMs : elapsedMs;
            std::uint32_t stepStart = 0;
            for (const auto& step : pattern.steps) {
                const auto duration = static_cast<std::uint32_t>(std::max(step.durationMs, 0));
                if (sequenceTime < stepStart + duration) {
                    state.color = step.color;
                    return state;
                }
                stepStart += duration;
            }
            state.color = pattern.steps.back().color;
            return state;
        }
    }

    return state;
}

LightColor LightEngine::scaledColor(const std::array<int, 3>& color, const int brightness) {
    const int level = clampByte(brightness);
    return {
        static_cast<std::uint8_t>(clampByte(color[0]) * level / kMaximumChannel),
        static_cast<std::uint8_t>(clampByte(color[1]) * level / kMaximumChannel),
        static_cast<std::uint8_t>(clampByte(color[2]) * level / kMaximumChannel),
    };
}

LightColor LightEngine::scaledColor(const LightPixel& pixel) {
    const int brightness = clampByte(static_cast<int>(pixel.intensity * static_cast<float>(kMaximumChannel)));
    return scaledColor(pixel.color, brightness);
}

void LightEngine::renderPatternState(const PatternState& state) {
    std::vector<LightColor> pixels(m_backend->ledCount(), scaledColor(state.color, state.brightness));
    renderPixels(std::move(pixels));
}

void LightEngine::renderSequenceFrame(const LightFrame& frame) {
    std::vector<LightColor> pixels(m_backend->ledCount(), LightColor{0, 0, 0});
    for (const auto& pixel : frame.pixels) {
        if (pixel.index < pixels.size()) {
            pixels[pixel.index] = scaledColor(pixel);
        }
    }
    renderPixels(std::move(pixels));
}

void LightEngine::renderPixels(std::vector<LightColor> pixels) {
    if (pixels == m_lastPixels) {
        return;
    }
    m_backend->show(pixels);
    m_lastPixels = std::move(pixels);
}

void LightEngine::clear() {
    renderPixels(std::vector<LightColor>(m_backend->ledCount(), LightColor{0, 0, 0}));
}
