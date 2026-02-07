cmake_minimum_required(VERSION 3.24)
project(tests)

include(Utils)
include(FetchCMocka)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

IncludeSubdirectories()
