cmake_minimum_required(VERSION 3.23)
project(raycaster LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

file(GLOB_RECURSE SOURCES src/*.c)
add_executable(raycaster ${SOURCES})

target_include_directories(raycaster PRIVATE include/)
target_link_libraries(raycaster PRIVATE SDL2 m)
