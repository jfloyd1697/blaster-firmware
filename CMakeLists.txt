cmake_minimum_required(VERSION 3.16)
project(blaster_firmware)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BLASTER_PLATFORM "PC" CACHE STRING "Target platform")
set_property(CACHE BLASTER_PLATFORM PROPERTY STRINGS PC ESP)

add_subdirectory(lib/core)
add_subdirectory(lib/esp8266_platform)

add_executable(BlasterApp
        src/main.cpp
)

if(BLASTER_PLATFORM STREQUAL "PC")
    target_compile_definitions(BlasterApp PRIVATE PLATFORM_PC)
    target_link_libraries(BlasterApp PRIVATE winmm)
elseif(BLASTER_PLATFORM STREQUAL "ESP")
    target_compile_definitions(BlasterApp PRIVATE PLATFORM_ESP)
endif()

target_include_directories(BlasterApp
        PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(BlasterApp
        PRIVATE
        core
        esp8266_platform
)