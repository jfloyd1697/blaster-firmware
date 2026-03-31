cmake_minimum_required(VERSION 3.16)

if(BLASTER_PLATFORM STREQUAL "ESP")
    add_library(blaster_platform STATIC
            platform/esp8266/ESPAudioEngine.cpp
    )

    target_link_libraries(blaster_platform
            PUBLIC
            core
            blaster_core
    )
elseif(BLASTER_PLATFORM STREQUAL "PC")
    add_library(blaster_platform INTERFACE
            ../include/platform/esp8266/ESPAudioEngine.h
            platform/esp8266/ESPAudioEngine.cpp)
endif()