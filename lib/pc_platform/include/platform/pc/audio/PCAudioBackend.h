#ifndef BLASTER_FIRMWARE_PCAUDIOBACKEND_H
#define BLASTER_FIRMWARE_PCAUDIOBACKEND_H

#pragma once

#include <string>

#include "audio/IAudioBackend.h"

struct IDebug;

class PCAudioBackend : public IAudioBackend {
public:
    explicit PCAudioBackend(IDebug *debug);

    bool begin() override;

    void update() override;

    void playSound(const std::string &file, bool loop = false, bool blocking = false) override;

    void stop() override;

private:
    IDebug *m_debug = nullptr;
};

#endif // BLASTER_FIRMWARE_PCAUDIOBACKEND_H
