file(GLOB_RECURSE STM32F411_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(stm32f411_platform ${STM32F411_SOURCES})

target_include_directories(stm32f411_platform
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(stm32f411_platform
        PUBLIC
        core
        audio
)

target_compile_definitions(stm32f411_platform PUBLIC PLATFORM_STM32F411)
target_compile_features(stm32f411_platform PUBLIC cxx_std_17)
