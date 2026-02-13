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

if(BUILD_TESTS)
  add_subdirectory(test)
endif()
