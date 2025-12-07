file(GLOB_RECURSE GAME_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(Game ${GAME_SOURCES})

# El juego usa el motor
target_link_libraries(Game PRIVATE Engine)