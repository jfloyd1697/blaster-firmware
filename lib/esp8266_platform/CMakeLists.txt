file(GLOB_RECURSE ESP_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(esp8266_platform ${ESP_SOURCES})

target_include_directories(esp8266_platform
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(esp8266_platform
        PUBLIC
        core
        audio
        lights
)

target_compile_definitions(esp8266_platform
        PUBLIC
        PLATFORM_ESP
)

target_compile_features(esp8266_platform PUBLIC cxx_std_17)