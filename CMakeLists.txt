cmake_minimum_required(VERSION 3.16)
project(blaster_firmware)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BLASTER_PLATFORM "PC" CACHE STRING "Target platform")
set_property(CACHE BLASTER_PLATFORM PROPERTY STRINGS PC ESP TEENSY STM32F411)

add_subdirectory(lib/weapon_behavior)
add_subdirectory(lib/core)
add_subdirectory(lib/pc_platform)
add_subdirectory(lib/esp8266_platform)

if(BLASTER_PLATFORM STREQUAL "TEENSY")
    add_subdirectory(lib/teensy_platform)
elseif(BLASTER_PLATFORM STREQUAL "STM32F411")
    add_subdirectory(lib/stm32f411_platform)
endif()

add_executable(BlasterApp
        src/main.cpp
)

if(BLASTER_PLATFORM STREQUAL "PC")
    target_compile_definitions(BlasterApp PRIVATE PLATFORM_PC)
    target_link_libraries(BlasterApp PRIVATE pc_platform)

    add_custom_command(TARGET BlasterApp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:BlasterApp>/assets
    )

elseif(BLASTER_PLATFORM STREQUAL "ESP")
    target_compile_definitions(BlasterApp PRIVATE PLATFORM_ESP)
    target_link_libraries(BlasterApp
            PRIVATE
            core
            esp8266_platform
    )

elseif(BLASTER_PLATFORM STREQUAL "TEENSY")
    target_compile_definitions(BlasterApp PRIVATE PLATFORM_TEENSY)
    target_link_libraries(BlasterApp
            PRIVATE
            core
            teensy_platform
    )

elseif(BLASTER_PLATFORM STREQUAL "STM32F411")
    target_compile_definitions(BlasterApp PRIVATE PLATFORM_STM32F411)
    target_link_libraries(BlasterApp
            PRIVATE
            core
            stm32f411_platform
    )
endif()

target_link_libraries(BlasterApp
        PRIVATE
        core
)
include(CTest)
if(BUILD_TESTING AND BLASTER_PLATFORM STREQUAL "PC")
    add_executable(WeaponSwitchCleanupTest tests/WeaponSwitchCleanupTest.cpp)
    target_link_libraries(WeaponSwitchCleanupTest PRIVATE core)
    add_test(NAME weapon_switch_cleanup COMMAND WeaponSwitchCleanupTest)
    set_tests_properties(weapon_switch_cleanup PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    add_executable(MergedLibraryTest tests/MergedLibraryTest.cpp)
    target_link_libraries(MergedLibraryTest PRIVATE core)
    add_test(NAME merged_library COMMAND MergedLibraryTest)
    set_tests_properties(merged_library PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    add_executable(AssetLayoutTest tests/AssetLayoutTest.cpp)
    target_link_libraries(AssetLayoutTest PRIVATE core)
    add_test(NAME asset_layout COMMAND AssetLayoutTest)
    set_tests_properties(asset_layout PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    add_executable(WeaponRuntimeTest tests/WeaponRuntimeTest.cpp)
    target_link_libraries(WeaponRuntimeTest PRIVATE core)
    add_test(NAME weapon_runtime COMMAND WeaponRuntimeTest)
    set_tests_properties(weapon_runtime PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    add_executable(ResourceTreeDumperTest tests/ResourceTreeDumperTest.cpp)
    target_link_libraries(ResourceTreeDumperTest PRIVATE core)
    add_test(NAME resource_tree_dumper COMMAND ResourceTreeDumperTest)

    add_executable(AnalogButtonLadderTest tests/AnalogButtonLadderTest.cpp)
    target_link_libraries(AnalogButtonLadderTest PRIVATE core)
    add_test(NAME analog_button_ladder COMMAND AnalogButtonLadderTest)

    add_executable(LightOutputCurveTest tests/LightOutputCurveTest.cpp)
    target_include_directories(LightOutputCurveTest PRIVATE lib/core/include)
    add_test(NAME light_output_curve COMMAND LightOutputCurveTest)

    add_executable(CompiledAssetsTest tests/CompiledAssetsTest.cpp)
    target_link_libraries(CompiledAssetsTest PRIVATE core)
    add_test(NAME compiled_assets COMMAND CompiledAssetsTest)
    set_tests_properties(compiled_assets PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    add_executable(TeensyToneBackendTest
        tests/TeensyToneBackendTest.cpp
        lib/teensy_platform/src/audio/TeensyAudioBackend.cpp
    )
    target_include_directories(TeensyToneBackendTest BEFORE PRIVATE
        tests/stubs
        lib/teensy_platform/include
    )
    target_link_libraries(TeensyToneBackendTest PRIVATE core)
    add_test(NAME teensy_tone_backend COMMAND TeensyToneBackendTest)

    add_executable(Stm32ToneBackendTest
        tests/Stm32ToneBackendTest.cpp
        lib/stm32f411_platform/src/audio/Stm32F411AudioBackend.cpp
    )
    target_include_directories(Stm32ToneBackendTest BEFORE PRIVATE
        tests/stubs
        lib/stm32f411_platform/include
    )
    target_link_libraries(Stm32ToneBackendTest PRIVATE core)
    add_test(NAME stm32_tone_backend COMMAND Stm32ToneBackendTest)

    add_executable(Stm32I2sClockConfigTest
        tests/Stm32I2sClockConfigTest.cpp
        lib/stm32f411_platform/src/audio/Stm32F411AudioBackend.cpp
    )
    target_include_directories(Stm32I2sClockConfigTest BEFORE PRIVATE
        tests/stubs
        lib/stm32f411_platform/include
    )
    target_link_libraries(Stm32I2sClockConfigTest PRIVATE core)
    add_test(NAME stm32_i2s_clock_config COMMAND Stm32I2sClockConfigTest)

    add_executable(Stm32I2sStartupRecoveryTest
        tests/Stm32I2sStartupRecoveryTest.cpp
        lib/stm32f411_platform/src/audio/Stm32F411AudioBackend.cpp
    )
    target_include_directories(Stm32I2sStartupRecoveryTest BEFORE PRIVATE
        tests/stubs
        lib/stm32f411_platform/include
    )
    target_link_libraries(Stm32I2sStartupRecoveryTest PRIVATE core)
    add_test(NAME stm32_i2s_startup_recovery COMMAND Stm32I2sStartupRecoveryTest)

    add_executable(Stm32LightBackendTest
        tests/Stm32LightBackendTest.cpp
        lib/stm32f411_platform/src/lights/Stm32F411LightBackend.cpp
    )
    target_include_directories(Stm32LightBackendTest BEFORE PRIVATE
        tests/stubs
        lib/stm32f411_platform/include
    )
    target_link_libraries(Stm32LightBackendTest PRIVATE core)
    add_test(NAME stm32_light_backend COMMAND Stm32LightBackendTest)

    add_executable(PlatformDiagnosticsTest
        tests/PlatformDiagnosticsTest.cpp
        src/tests/PlatformDiagnostics.cpp
    )
    target_include_directories(PlatformDiagnosticsTest BEFORE PRIVATE tests/stubs)
    target_compile_definitions(PlatformDiagnosticsTest PRIVATE PLATFORM_STM32F411)
    target_link_libraries(PlatformDiagnosticsTest PRIVATE core)
    add_test(NAME platform_diagnostics COMMAND PlatformDiagnosticsTest)
    set_tests_properties(platform_diagnostics PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    add_executable(Stm32I2sCounterHalTest
        tests/Stm32I2sCounterHalTest.cpp
        lib/stm32f411_platform/src/diagnostics/I2sGpioLoopback.cpp
    )
    target_include_directories(Stm32I2sCounterHalTest BEFORE PRIVATE
        lib/stm32f411_platform/include
        tests/stubs
    )
    add_test(NAME stm32_i2s_counter_hal COMMAND Stm32I2sCounterHalTest)

    add_executable(I2sGpioLoopbackMathTest tests/I2sGpioLoopbackMathTest.cpp)
    target_include_directories(I2sGpioLoopbackMathTest PRIVATE lib/stm32f411_platform/include)
    add_test(NAME i2s_gpio_loopback_math COMMAND I2sGpioLoopbackMathTest)

    add_executable(ToneGeneratorTest tests/ToneGeneratorTest.cpp)
    target_link_libraries(ToneGeneratorTest PRIVATE core)
    add_test(NAME tone_generator COMMAND ToneGeneratorTest)

    add_executable(LightEngineTest tests/LightEngineTest.cpp)
    target_link_libraries(LightEngineTest PRIVATE core)
    add_test(NAME light_engine COMMAND LightEngineTest)
endif()
