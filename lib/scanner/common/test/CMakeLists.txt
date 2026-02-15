add_library(TestScannerCommon SHARED tokens.c)

set_target_properties(
  TestScannerCommon
  
  PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
    COMPILE_WARNING_AS_ERROR TRUE
)

target_compile_features(TestScannerCommon PUBLIC c_std_23)

target_link_libraries(
  TestScannerCommon
  PUBLIC
    cmocka
    cloxcommon
    cloxscanner_common
)
