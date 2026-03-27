cmake_minimum_required(VERSION 3.20)
project(BlasterFirmware)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add subdirectories
add_subdirectory(core)
add_subdirectory(platform/pc)
add_subdirectory(app)
