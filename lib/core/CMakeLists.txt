cmake_minimum_required(VERSION 3.16)

add_library(core STATIC
        Blaster.cpp
        weapons/WeaponJSONLoader.cpp
        lights/LightEngine.cpp
        input/IInput.cpp
)

target_include_directories(core
        PUBLIC
        .
        ${CMAKE_SOURCE_DIR}/external/nlohmann
)

target_link_libraries(core
        PUBLIC
        blaster_core
)