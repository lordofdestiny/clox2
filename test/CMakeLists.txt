cmake_minimum_required(VERSION 3.24)
project(clox_tests)

option(CLOX_INSTALL_GTEST "Installing gtest..." ON)

if(NOT CLOX_INSTALL_GTEST)
	find_package(GTest REQUIRED)
else()
	include(FetchContent)
	FetchContent_Declare(
  		googletest
  		GIT_REPOSITORY https://github.com/google/googletest.git
  		GIT_TAG v1.17.0
	)
endif()

include(GoogleTest)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_subdirectory(misc)
