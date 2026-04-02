file(GLOB_RECURSE AUDIO_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(audio ${AUDIO_SOURCES})

target_include_directories(audio
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(audio
        PUBLIC
        core
)

target_compile_features(audio PUBLIC cxx_std_17)