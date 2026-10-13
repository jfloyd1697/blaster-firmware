#pragma once

#ifdef PLATFORM_STM32F411

#include <Arduino.h>
#include <SdFat.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include "BridgeProtocol.h"
#include "core/Platform.h"

class Stm32Bridge {
public:
    enum class Mode : std::uint8_t { Local = 0, RemoteInput = 1, DirectControl = 2 };

    explicit Stm32Bridge(PlatformServices& services);
    void service();
    bool directControl() const { return m_mode == Mode::DirectControl; }
    bool maintenanceActive() const { return m_maintenance; }
    bool takeActivate();
    void finishedActivation();

private:
    void command(const bridge::Frame& frame);
    void acknowledge(std::uint16_t sequence, bridge::Result result,
                     const std::uint8_t* extra = nullptr, std::size_t extraSize = 0);
    void reply(bridge::Op op, std::uint16_t sequence, const std::uint8_t* payload, std::size_t size);
    void setMode(Mode mode);
    void enterMaintenance();
    void leaveMaintenance();
    void abortUpload();
    void serviceHash();
    bridge::Result fileCommand(const bridge::Frame& frame, std::uint8_t* result, std::size_t& resultSize);
    bool validPath(const std::string& path, bool rootAllowed = false) const;
    bool decodePath(const std::uint8_t* data, std::size_t size, std::string& path,
                    bool rootAllowed = false) const;

    PlatformServices& m_services;
    bridge::Parser m_parser;
    Mode m_mode = Mode::Local;
    bool m_maintenance = false;
    bool m_activate = false;
    std::uint32_t m_lastHeartbeatMs = 0;

    File32 m_upload;
    std::string m_targetPath;
    std::string m_stagePath;
    std::uint32_t m_expectedSize = 0;
    std::uint32_t m_receivedSize = 0;
    std::uint32_t m_uploadCrc = 0xFFFFFFFFu;

    File32 m_hashFile;
    bool m_hashPending = false;
    std::uint16_t m_hashSequence = 0;
    std::uint32_t m_hashSize = 0;
    std::uint32_t m_hashCrc = 0xFFFFFFFFu;
};

#endif
