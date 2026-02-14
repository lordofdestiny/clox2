add_executable(TestScannerDefault)
add_executable(TestScannerFlex)

file(GLOB_RECURSE TARGET_SOURCES "*.c")

target_sources(TestScannerDefault
  PRIVATE
    ${TARGET_SOURCES}
)

target_sources(TestScannerFlex
  PRIVATE
    ${TARGET_SOURCES}
)

set_target_properties(
  TestScannerDefault
  TestScannerFlex
  
  PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
    COMPILE_WARNING_AS_ERROR TRUE
)

target_compile_features(TestScannerDefault PRIVATE c_std_23)

target_compile_features(TestScannerFlex PRIVATE c_std_23)

target_link_libraries(
  TestScannerDefault
  PUBLIC
    cmocka
    cloxcommon
    cloxscanner
)

target_link_libraries(
  TestScannerFlex
  PUBLIC
    cmocka
    cloxcommon
    cloxscanner_flex
)

add_test(NAME TestScannerDefault COMMAND TestScannerDefault)
add_test(NAME TestScannerFlex COMMAND TestScannerFlex)
