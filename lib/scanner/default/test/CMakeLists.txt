add_executable(TestScannerDefault)

set_target_properties(
  TestScannerDefault
  
  PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
    COMPILE_WARNING_AS_ERROR TRUE
)

target_link_libraries(
  TestScannerDefault
  PUBLIC
    TestScannerCommon
    cloxcommon
    cloxscanner_default
)

add_test(NAME TestScannerDefault COMMAND TestScannerDefault)
