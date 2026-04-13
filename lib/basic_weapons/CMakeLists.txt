file(GLOB_RECURSE BASIC_WEAPONS_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(basic_weapons ${BASIC_WEAPONS_SOURCES})

target_include_directories(basic_weapons
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(basic_weapons PUBLIC core)
