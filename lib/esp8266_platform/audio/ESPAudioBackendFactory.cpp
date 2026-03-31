#include "ESPAudioBackendFactory.h"

#include "IAudioEngine.h"
#include "audio/AudioEngine.h"
#include "BasicESPAudioBackend.h"
#include "OverlapESPAudioBackend.h"
#include "PseudoOverlapESPAudioBackend.h"

std::unique_ptr<IAudioEngine> createESPAudioEngine(
    const ESPAudioBackendType type,
    IDebug* debug,
    ITime* time,
    const std::size_t overlapVoices,
    const std::uint32_t pseudoOverlapMs
) {
    switch (type) {
        case ESPAudioBackendType::Basic:
            return std::make_unique<AudioEngine>(
                std::make_unique<BasicESPAudioBackend>(debug)
            );

        case ESPAudioBackendType::Overlap:
            return std::make_unique<AudioEngine>(
                std::make_unique<OverlapESPAudioBackend>(overlapVoices, debug)
            );

        case ESPAudioBackendType::PseudoOverlap:
            return std::make_unique<AudioEngine>(
                std::make_unique<PseudoOverlapESPAudioBackend>(time, debug, pseudoOverlapMs)
            );
    }

    return nullptr;
}