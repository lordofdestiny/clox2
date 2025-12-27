add_library(args OBJECT)

target_sources(args
  PRIVATE
    args.c

  PUBLIC
    FILE_SET HEADERS
    FILES
      include/args.h
)

target_include_directories(args PUBLIC include)
target_link_libraries(args PUBLIC m)