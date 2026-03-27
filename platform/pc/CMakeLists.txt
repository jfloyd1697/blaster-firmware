# Define header-only interface library
add_library(platform_pc INTERFACE
        PCPlatform.h
)

# Include path for the PC platform headers
target_include_directories(platform_pc
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}  # PCPlatform.h lives here
)

# Link Windows multimedia library (PlaySound)
target_link_libraries(platform_pc
        INTERFACE winmm
)