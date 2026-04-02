cmake_minimum_required(VERSION 3.16)
project(blaster_firmware)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BLASTER_PLATFORM "PC" CACHE STRING "Target platform")
set_property(CACHE BLASTER_PLATFORM PROPERTY STRINGS PC ESP)

add_subdirectory(lib/weapon_behavior)
add_subdirectory(lib/core)
add_subdirectory(lib/audio)
add_subdirectory(lib/pc_platform)
add_subdirectory(lib/esp8266_platform)

add_executable(BlasterApp
        src/main.cpp
)

if(BLASTER_PLATFORM STREQUAL "PC")
    target_compile_definitions(BlasterApp PRIVATE PLATFORM_PC)
    target_link_libraries(BlasterApp PRIVATE pc_platform)

    add_custom_command(TARGET BlasterApp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:BlasterApp>/assets
    )

elseif(BLASTER_PLATFORM STREQUAL "ESP")
    target_compile_definitions(BlasterApp PRIVATE PLATFORM_ESP)
    target_link_libraries(BlasterApp PRIVATE esp8266_platform)
endif()

target_link_libraries(BlasterApp
        PRIVATE
        core
        audio
)