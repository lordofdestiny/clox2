cmake_minimum_required(VERSION 3.24)
project(tests)

include(Utils)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

IncludeSubdirectories()
