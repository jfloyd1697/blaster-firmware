cmake_minimum_required(VERSION 3.16)

# Create executable
add_executable(BlasterApp
        main.cpp
        ../platform/pc/PCPlatform.h
#        test_input.cpp
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