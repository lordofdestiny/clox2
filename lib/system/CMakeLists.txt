include(GenerateExportHeader)
include(CloxNative)

add_library(cloxsystem MODULE)

set(SPEC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/spec.json")

CloxNativeLibrary(
  LIB_NAME system
  TARGET_NAME cloxsystem
  SPEC_FILE ${SPEC_FILE} 
  OUT_HEADER WRAPPER_HEADER
  OUT_SOURCE WRAPPER_SOURCE
)

set_target_properties(
  cloxsystem
  PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    COMPILE_WARNING_AS_ERROR TRUE
)
