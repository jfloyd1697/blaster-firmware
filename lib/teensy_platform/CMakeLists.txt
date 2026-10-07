file(GLOB_RECURSE TEENSY_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(teensy_platform ${TEENSY_SOURCES})

target_include_directories(teensy_platform
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(teensy_platform
        PUBLIC
        core
        audio
)

target_compile_definitions(teensy_platform
        PUBLIC
        PLATFORM_TEENSY
)

target_compile_features(teensy_platform PUBLIC cxx_std_17)
