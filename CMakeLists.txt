cmake_minimum_required(VERSION 3.24)
project(clox LANGUAGES C)

option(BUILD_TESTS "Include testing targets during build" ON)

find_package(Python3 3.12 REQUIRED COMPONENTS Interpreter)

if(Python3_FOUND)
    set(PythonString "Python ${Python3_VERSION}")
    message(STATUS "Found ${PythonString}")
    message(STATUS "${PythonString} executable: ${Python3_EXECUTABLE}")
    unset(PythonString)
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include_directories(include)

add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
