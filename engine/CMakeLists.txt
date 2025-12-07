# Recolectar todos los .cpp del motor
file(GLOB_RECURSE ENGINE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_library(Engine ${ENGINE_SOURCES})

# Headers públicos del motor
target_include_directories(Engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# SDL3 para el motor
target_link_libraries(Engine PUBLIC SDL3::SDL3)