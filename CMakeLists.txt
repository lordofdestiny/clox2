cmake_minimum_required(VERSION 3.24)
project(clox LANGUAGES C)

option(BUILD_TESTS "Include testing targets during build" YES)
option(ENABLE_COVERAGE "Enable code coverage generation" NO)

find_package(Python3 3.12 REQUIRED COMPONENTS Interpreter)

if(Python3_FOUND)
    set(PythonString "Python ${Python3_VERSION}")
    message(STATUS "Found ${PythonString}")
    message(STATUS "${PythonString} executable: ${Python3_EXECUTABLE}")
    unset(PythonString)
endif()

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_C_FLAGS "-pedantic-errors -Wpedantic -pedantic -Wall -Wextra")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(BUILD_TESTS)
    if(ENABLE_COVERAGE)
        # Check for compatible compilers (GCC and Clang support --coverage flag)
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            add_compile_options("--coverage" "-O0" "-g") # Disable optimization for accurate coverage
            add_link_options("--coverage")
        else()
            message(WARNING "Code coverage not supported for this compiler")
        endif()
    endif()

    enable_testing()
    add_subdirectory(test)
endif()

add_subdirectory(src)

