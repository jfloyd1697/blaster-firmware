cmake_minimum_required(VERSION 3.16)
project(blaster_firmware)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform selection
set(BLASTER_PLATFORM "PC" CACHE STRING "Target platform")
set_property(CACHE BLASTER_PLATFORM PROPERTY STRINGS PC ESP)

# Submodules
add_subdirectory(lib/core)
add_subdirectory(lib/audio)

if(BLASTER_PLATFORM STREQUAL "PC")
#    add_subdirectory(lib/pc_platform)
elseif(BLASTER_PLATFORM STREQUAL "ESP")
    add_subdirectory(lib/esp8266_platform)
endif()

# Main executable
add_executable(BlasterApp
        src/main.cpp
)

# Platform-specific defines
if(BLASTER_PLATFORM STREQUAL "PC")
    target_compile_definitions(BlasterApp PRIVATE PLATFORM_PC)
    target_link_libraries(BlasterApp PRIVATE winmm)
elseif(BLASTER_PLATFORM STREQUAL "ESP")
    target_compile_definitions(BlasterApp PRIVATE PLATFORM_ESP)
endif()

# Shared includes
target_include_directories(BlasterApp
        PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Link everything
target_link_libraries(BlasterApp
        PRIVATE
        core
        audio
)
if(BLASTER_PLATFORM STREQUAL "ESP")
    target_link_libraries(BlasterApp PRIVATE esp8266_platform)
endif()