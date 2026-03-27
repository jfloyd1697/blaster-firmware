cmake_minimum_required(VERSION 3.16)

# Create executable
add_executable(BlasterApp
        main.cpp
        ../platform/pc/PCPlatform.h
)

# Include core and platform headers
target_include_directories(BlasterApp
        PRIVATE
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/platform/pc
)

# Link libraries
target_link_libraries(BlasterApp
        PRIVATE core
        PRIVATE platform_pc
        PRIVATE winmm  # link Windows multimedia library for PlaySound
)

# Copy assets folder to the output directory after build
add_custom_command(TARGET BlasterApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:BlasterApp>/assets
        COMMENT "Copying assets folder to build output"
)


# Create executable
add_executable(TestInput
        test_input.cpp
        ../platform/pc/PCPlatform.h
)

# Include core and platform headers
target_include_directories(TestInput
        PRIVATE
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/platform/pc
)

# Link libraries
target_link_libraries(TestInput
        PRIVATE core
        PRIVATE platform_pc
        PRIVATE winmm  # link Windows multimedia library for PlaySound
)

# Copy assets folder to the output directory after build
add_custom_command(TARGET TestInput POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:TestInput>/assets
        COMMENT "Copying assets folder to build output"
)

# Include directories for SDL2, ImGui, and core
include_directories(
        ${SDL2_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/gui
)

add_executable(TestLights
        test_lights.cpp
        ${CMAKE_SOURCE_DIR}/gui/backends/imgui_impl_sdl2.cpp
        ${CMAKE_SOURCE_DIR}/gui/backends/imgui_impl_sdlrenderer2.cpp
        ${CMAKE_SOURCE_DIR}/gui/imgui.cpp
        ${CMAKE_SOURCE_DIR}/gui/imgui_draw.cpp
        ${CMAKE_SOURCE_DIR}/gui/imgui_widgets.cpp
        ${CMAKE_SOURCE_DIR}/gui/imgui_tables.cpp
)


find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(TestLights
        PRIVATE
        core
        platform_pc
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)