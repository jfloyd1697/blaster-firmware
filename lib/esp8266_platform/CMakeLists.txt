file(GLOB_RECURSE ESP_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(esp8266_platform ${ESP_SOURCES})


add_library(esp8266_intellisense INTERFACE)

set(PLATFORMIO_HOME "C:/Users/user/.platformio")
set(PIO_ENV "esp")
set(ARDUINO_ESP8266_FRAMEWORK "${PLATFORMIO_HOME}/packages/framework-arduinoespressif8266")
set(ESP8266_AUDIO_LIB "${CMAKE_SOURCE_DIR}/.pio/libdeps/${PIO_ENV}/ESP8266Audio")

target_compile_definitions(esp8266_intellisense INTERFACE
        PLATFORM_ESP
        ARDUINO=10805
        ESP8266
)

target_include_directories(esp8266_intellisense INTERFACE
        ${CMAKE_SOURCE_DIR}/include
        ${ARDUINO_ESP8266_FRAMEWORK}/cores/esp8266
        ${ARDUINO_ESP8266_FRAMEWORK}/libraries/SD/src
        ${ARDUINO_ESP8266_FRAMEWORK}/libraries/SPI
        ${ARDUINO_ESP8266_FRAMEWORK}/variants/nodemcu
        ${ESP8266_AUDIO_LIB}/src
)


target_include_directories(esp8266_platform
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(esp8266_platform
        PUBLIC
        core
        audio
        esp8266_intellisense
)

target_compile_definitions(esp8266_platform
        PUBLIC
        PLATFORM_ESP
)
message(STATUS "esp8266_platform include dir: ${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(esp8266_platform PUBLIC cxx_std_17)