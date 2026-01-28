include(FetchJannson)

add_executable(cloxn)

set_target_properties(
  cloxn
  PROPERTIES
    COMPILE_WARNING_AS_ERROR TRUE
)

target_sources(cloxn
  PRIVATE
    main.c
    config.c
    generate.c

  PRIVATE
  FILE_SET HEADERS
  BASE_DIRS include
  FILES
    include/config.h
)

target_link_libraries(cloxn
  PUBLIC
    jansson::jansson
)
