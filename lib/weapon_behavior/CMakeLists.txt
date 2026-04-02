add_library(weapon_behavior
        src/WeaponBehaviorParser.cpp
        src/WeaponBehaviorValidation.cpp
)

target_include_directories(weapon_behavior
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/external
)

target_compile_features(weapon_behavior PUBLIC cxx_std_17)