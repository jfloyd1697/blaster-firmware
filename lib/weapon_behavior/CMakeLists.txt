file(GLOB_RECURSE WEAPON_BEHAVIOR_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(weapon_behavior ${WEAPON_BEHAVIOR_SOURCES})

target_include_directories(weapon_behavior
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/lib/core/include
        ${CMAKE_SOURCE_DIR}/external
)

target_compile_features(weapon_behavior PUBLIC cxx_std_17)