cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE LIGHTS_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(audio ${LIGHTS_SOURCES})

target_include_directories(lights
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(audio PUBLIC cxx_std_17)