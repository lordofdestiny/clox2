add_executable(clox)

file(GLOB_RECURSE TARGET_SOURCES "*.c") 
file(GLOB_RECURSE TARGET_HEADERS "include/*.h")

target_sources(clox
  PRIVATE
    ${TARGET_SOURCES}

  PRIVATE
  FILE_SET HEADERS
  BASE_DIRS include
  FILES  
    ${TARGET_HEADERS}
)

set_target_properties(
  clox
  PROPERTIES
    COMPILE_WARNING_AS_ERROR TRUE
)

target_link_libraries(clox
  PRIVATE
    cloximpl
    cloxargs
    cloxcommon
)
