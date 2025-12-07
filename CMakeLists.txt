cmake_minimum_required(VERSION 3.23)
project(EntityEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL3_DIR "${CMAKE_SOURCE_DIR}/external/SDL3/cmake")

# Paquete oficial de SDL3
find_package(SDL3 REQUIRED CONFIG)

# Subproyectos
add_subdirectory(engine)
add_subdirectory(game)