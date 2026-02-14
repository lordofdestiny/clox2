include(GenerateExportHeader)
include(CloxNative)

add_library(cloxmath MODULE)

set(SPEC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/spec.json")

CloxNativeLibrary(TARGET_NAME cloxmath)

set_target_properties(
  cloxmath
  PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    COMPILE_WARNING_AS_ERROR TRUE
)
