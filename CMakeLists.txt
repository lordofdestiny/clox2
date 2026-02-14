cmake_minimum_required(VERSION 3.24)
project(clox LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(CheckIPOSupported)
include(CTest)

option(ENABLE_COVERAGE "Enable code coverage generation" NO)
option(CLOX_NAN_BOXING "Enable NAN BOXING for stack values" YES)


if(BUILD_TESTING)
  include(FetchCMocka)
  enable_testing()
endif()

find_package(Python3 3.12 REQUIRED COMPONENTS Interpreter)

if(Python3_FOUND)
    set(PythonString "Python ${Python3_VERSION}")
    message(STATUS "Found ${PythonString}")
    message(STATUS "${PythonString} executable: ${Python3_EXECUTABLE}")
    unset(PythonString)
endif()

set(CMAKE_C_EXTENSIONS ON)

check_ipo_supported(RESULT result OUTPUT output)
if(result)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(WARNING "IPO is not supported: ${output}")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic-errors -Wpedantic -pedantic -Wall -Wextra")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(BUILD_TESTING AND ENABLE_COVERAGE)
    # Check for compatible compilers (GCC and Clang support --coverage flag)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options("--coverage" "-O0" "-g") # Disable optimization for accurate coverage
        add_link_options("--coverage")
    else()
        message(WARNING "Code coverage not supported for this compiler")
    endif()
endif()
    
add_subdirectory(lib)
add_subdirectory(apps)
