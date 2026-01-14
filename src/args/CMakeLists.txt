add_library(cloxargs OBJECT)

target_sources(cloxargs
  PRIVATE
    args.c

  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
      include/args.h
)

target_link_libraries(cloxargs PUBLIC m)
