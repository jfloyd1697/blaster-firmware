#include "IAudioEngine.h"
#include "platform/esp8266/audio/ESPAudioBackendFactory.h"
#include "platform/esp8266/audio/BasicESPAudioBackend.h"
#include "platform/esp8266/audio/OverlapESPAudioBackend.h"
#include "platform/esp8266/audio/PseudoOverlapESPAudioBackend.h"
#include "platform/esp8266/audio/ESPAudioEngine.h"

std::unique_ptr<IAudioEngine> createESPAudioEngine(
    const ESPAudioBackendType type,
    IDebug* debug,
    ITime* time,
    const std::size_t overlapVoices,
    const uint32_t pseudoOverlapMs
) {
    switch (type) {
        case ESPAudioBackendType::Basic:
            return std::make_unique<ESPAudioEngine>(
                std::make_unique<BasicESPAudioBackend>(debug)
            );

        case ESPAudioBackendType::Overlap:
            return std::make_unique<ESPAudioEngine>(
                std::make_unique<OverlapESPAudioBackend>(overlapVoices, debug)
            );

        case ESPAudioBackendType::PseudoOverlap:
            return std::make_unique<ESPAudioEngine>(
                std::make_unique<PseudoOverlapESPAudioBackend>(time, debug, pseudoOverlapMs)
            );
    }

    return nullptr;
}
