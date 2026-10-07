file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(core ${CORE_SOURCES})

target_include_directories(core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/external
)

target_link_libraries(core
    PUBLIC
        weapon_behavior
)

target_compile_features(core PUBLIC cxx_std_17)
