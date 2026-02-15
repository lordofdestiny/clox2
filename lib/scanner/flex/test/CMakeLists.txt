add_executable(TestScannerFlex)

set_target_properties(
  TestScannerFlex
  
  PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
    COMPILE_WARNING_AS_ERROR TRUE
)

target_link_libraries(
  TestScannerFlex
  PUBLIC
    TestScannerCommon
    cloxcommon
    cloxscanner_flex
)

add_test(NAME TestScannerFlex COMMAND TestScannerFlex)
