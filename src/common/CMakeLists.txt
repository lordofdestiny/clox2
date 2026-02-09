add_library(cloxcommon OBJECT)

set_target_properties(
  cloxcommon
  PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    COMPILE_WARNING_AS_ERROR TRUE
)

target_sources(cloxcommon
  PRIVATE
    inputfile.c
    arena.c

  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
      include/inputfile.h
      include/arena.h
      include/util.h
)
