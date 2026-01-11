add_executable(ScannerTests)

file(GLOB_RECURSE TARGET_SOURCES "*.c")

target_sources(
  ScannerTests
  PRIVATE
    ${TARGET_SOURCES}
)

if(NOT USE_FLEX_SCANNER)
  set(scanner_lib cloxscanner)
else()
  set(scanner_lib cloxscanner_flex)
endif()

target_link_libraries(
  ScannerTests
  PRIVATE
    ${scanner_lib}
    cmocka
)

add_test(NAME ScannerTests COMMAND ScannerTests)
