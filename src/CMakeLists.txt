cmake_minimum_required(VERSION 3.16)

if(BLASTER_PLATFORM STREQUAL "ESP")
    add_library(blaster_platform STATIC
            ../lib/esp8266_platform/src/audio/ESPAudioEngine.cpp
    )
#elseif(BLASTER_PLATFORM STREQUAL "PC")
endif()

target_link_libraries(blaster_platform
        PUBLIC
        core
        blaster_core
)
