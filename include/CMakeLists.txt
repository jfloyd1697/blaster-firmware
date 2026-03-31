cmake_minimum_required(VERSION 3.16)

add_library(blaster_core INTERFACE
        platform/esp8266/ESPPlatform.h)

target_include_directories(blaster_core INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)