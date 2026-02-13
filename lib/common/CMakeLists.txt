add_library(cloxcommon OBJECT)

set_target_properties(
  cloxcommon
  PROPERTIES
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    POSITION_INDEPENDENT_CODE TRUE
    COMPILE_WARNING_AS_ERROR TRUE
)

target_sources(cloxcommon
  PRIVATE
    src/inputfile.c
    src/arena.c

  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
      include/common/inputfile.h
      include/common/arena.h
      include/common/ilist.h
      include/common/util.h
)
