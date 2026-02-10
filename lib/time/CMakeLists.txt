include(GenerateExportHeader)
include(CloxNative)

add_library(cloxtime MODULE)
set(SPEC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/spec.json")

CloxNativeLibrary(
  LIB_NAME time
  TARGET_NAME cloxtime
  SPEC_FILE ${SPEC_FILE} 
  OUT_HEADER WRAPPER_HEADER
  OUT_SOURCE WRAPPER_SOURCE
)

set_target_properties(
  cloxtime
  PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    COMPILE_WARNING_AS_ERROR TRUE
)
