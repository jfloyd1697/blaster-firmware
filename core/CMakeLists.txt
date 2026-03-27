cmake_minimum_required(VERSION 3.16)
project(core)

add_library(core
        Blaster.cpp
        WeaponJSONLoader.cpp
        IDebug.h
)

# Include paths
target_include_directories(core
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}           # core headers
        ${CMAKE_SOURCE_DIR}/external/nlohmann # nlohmann/json.hpp
)