include(FetchJannson)

add_executable(cloxnative)

set_target_properties(
  cloxnative
  PROPERTIES
    COMPILE_WARNING_AS_ERROR TRUE
)

target_sources(cloxnative
  PRIVATE
    main.c
    config.c

  PRIVATE
  FILE_SET HEADERS
  BASE_DIRS include
  FILES
    include/config.h
)

target_link_libraries(cloxnative
  PUBLIC
    jansson::jansson
)
