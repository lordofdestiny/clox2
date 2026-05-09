set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/externals/cmake-argp/cmake" "${CMAKE_MODULE_PATH}")

find_package(argp REQUIRED)

add_library(cloxargs OBJECT)

target_sources(cloxargs
  PRIVATE
    src/args.c

  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
      include/args.h
)

target_link_libraries(cloxargs PRIVATE argp)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
