cmake_minimum_required(VERSION 3.16)
project(blaster_firmware)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional platform flag
set(BLASTER_PLATFORM "PC" CACHE STRING "Target platform")
set_property(CACHE BLASTER_PLATFORM PROPERTY STRINGS PC ESP)

add_subdirectory(include)
add_subdirectory(core)
add_subdirectory(src)

add_executable(BlasterApp
        src/main.cpp
)

target_link_libraries(BlasterApp
        PRIVATE
        core
        blaster_core
)

add_custom_command(TARGET BlasterApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:BlasterApp>/assets
        COMMENT "Copying assets folder to build output"
)

# Platform define
if(BLASTER_PLATFORM STREQUAL "PC")
    target_compile_definitions(BlasterApp PRIVATE PLATFORM_PC)
    target_link_libraries(BlasterApp PRIVATE winmm)
elseif(BLASTER_PLATFORM STREQUAL "ESP")
    target_compile_definitions(BlasterApp PRIVATE PLATFORM_ESP)
endif()
