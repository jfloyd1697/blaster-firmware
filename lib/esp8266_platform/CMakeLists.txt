cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE ESP8266_PLATFORM_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(esp8266_platform ${ESP8266_PLATFORM_SOURCES})

target_include_directories(esp8266_platform
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(esp8266_platform
        PUBLIC
        core
        audio
)

target_compile_definitions(esp8266_platform
        PUBLIC
        PLATFORM_ESP
)

target_compile_features(esp8266_platform PUBLIC cxx_std_17)