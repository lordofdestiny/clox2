set(SCANNER_SCRIPT misc/generateScanner.py)
set(SCANER_SPEC_FILE misc/scanner_spec.toml)

add_library(cloxscanner_default OBJECT)

file(GLOB_RECURSE TARGET_SOURCES "src/*")

target_sources(
  cloxscanner_default
  PRIVATE
    ${TARGET_SOURCES}
)

set_target_properties(
  cloxscanner_default
  PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    C_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN YES 
    COMPILE_WARNING_AS_ERROR TRUE
)

target_compile_features(cloxscanner_default PRIVATE c_std_23)

target_link_libraries(
  cloxscanner_default
  PUBLIC
    cloxcommon
    cloxscanner_common
)

add_subdirectory(test)
