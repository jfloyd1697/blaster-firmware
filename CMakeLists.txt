cmake_minimum_required(VERSION 3.16)
project(blaster_firmware)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BLASTER_PLATFORM "PC" CACHE STRING "Target platform")
set_property(CACHE BLASTER_PLATFORM PROPERTY STRINGS PC ESP)

add_subdirectory(include)
add_subdirectory(lib/core)

add_executable(BlasterApp
        src/main.cpp
)

target_link_libraries(BlasterApp
        PRIVATE
        core
        blaster_core
)

target_include_directories(BlasterApp
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/lib/core
        ${CMAKE_SOURCE_DIR}/external
)

if(BLASTER_PLATFORM STREQUAL "PC")
    target_compile_definitions(BlasterApp PRIVATE PLATFORM_PC)
    target_link_libraries(BlasterApp PRIVATE winmm)

elseif(BLASTER_PLATFORM STREQUAL "ESP")
    target_compile_definitions(BlasterApp PRIVATE PLATFORM_ESP)

    add_subdirectory(lib/esp8266_platform)
    target_link_libraries(BlasterApp PRIVATE esp8266_platform)

    set(PIO_FRAMEWORK_DIR "${CMAKE_SOURCE_DIR}/.pio/packages/framework-arduinoespressif8266")
    set(PIO_AUDIO_DIR "${CMAKE_SOURCE_DIR}/.pio/libdeps/esp/ESP8266Audio")

    if(EXISTS "${PIO_FRAMEWORK_DIR}" AND EXISTS "${PIO_AUDIO_DIR}")
        target_include_directories(BlasterApp PRIVATE
                ${PIO_AUDIO_DIR}/src
                ${PIO_FRAMEWORK_DIR}/cores/esp8266
                ${PIO_FRAMEWORK_DIR}/variants/nodemcu
                ${PIO_FRAMEWORK_DIR}/libraries/SD/src
                ${PIO_FRAMEWORK_DIR}/libraries/SPI
                ${PIO_FRAMEWORK_DIR}/libraries/SPI/src
        )
    endif()
endif()
