add_executable(MiscTests)

file(GLOB_RECURSE MiscTests_SRC "*.cpp")

target_sources(MiscTests
  PRIVATE
    ${MiscTests_SRC}
    ${clox_SOURCE_DIR}/bin/src/args.c
)

target_include_directories(MiscTests
  PRIVATE
    ${clox_SOURCE_DIR}/bin/include
    ${clox_SOURCE_DIR}/lib/include
)

target_link_libraries(MiscTests
  PRIVATE
	GTest::gtest_main
)

gtest_discover_tests(MiscTests)
