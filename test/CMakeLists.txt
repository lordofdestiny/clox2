cmake_minimum_required(VERSION 3.24)
project(tests)

option(CLOX_INSTALL_GTEST "Installing gtest..." OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

if(CLOX_INSTALL_GTEST)
	include(FetchContent)
	FetchContent_Declare(
		googletest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG v1.17.0
	)
	FetchContent_MakeAvailable(googletest)
else()
	find_package(GTest REQUIRED)
endif()

include(GoogleTest)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(misc)
