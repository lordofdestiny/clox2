cmake_minimum_required(VERSION 3.24)
project(clox LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(test)
add_subdirectory(bin)
add_subdirectory(lib)
