file(GLOB_RECURSE PC_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(pc_platform ${PC_SOURCES})

target_include_directories(pc_platform
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(pc_platform
        PUBLIC
        core
        audio
        lights
)

target_compile_definitions(pc_platform
        PUBLIC
        PLATFORM_PC
)

target_compile_features(pc_platform PUBLIC cxx_std_17)