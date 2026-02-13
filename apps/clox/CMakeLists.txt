add_subdirectory(args)

add_executable(clox)

file(GLOB_RECURSE TARGET_SOURCES "src/*") 

target_sources(clox
  PRIVATE
    ${TARGET_SOURCES}
)

set_target_properties(
  clox
  PROPERTIES
    COMPILE_WARNING_AS_ERROR TRUE
    BUILD_RPATH "${PROJECT_BINARY_DIR}/lib"
)

target_link_libraries(clox
  PRIVATE
    cloximpl
    cloxargs
    cloxcommon
)
