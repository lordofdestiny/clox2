add_library(cloxcommon OBJECT)

set_target_properties(
  cloxcommon
  PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    COMPILE_WARNING_AS_ERROR TRUE
)

target_compile_features(cloxcommon PUBLIC c_std_23)

target_sources(cloxcommon
  PRIVATE
    src/arena.c
    src/inputfile.c
    src/xerror.c

  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
      include/common/arena.h
      include/common/inputfile.h
      include/common/ilist.h
      include/common/util.h
      include/common/xerror.h
)

add_subdirectory(test)
