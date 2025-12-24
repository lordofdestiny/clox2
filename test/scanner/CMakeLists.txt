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

set_target_properties(TestScannerDefault TestScannerFlex
  PROPERTIES
    COMPILE_WARNING_AS_ERROR TRUE
)

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
