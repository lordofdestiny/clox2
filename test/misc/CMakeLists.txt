add_executable(MiscTests)

file(GLOB_RECURSE MiscTests_SRC "*.cpp")

target_sources(MiscTests
  PRIVATE
    ${MiscTests_SRC}
)

target_include_directories(MiscTests
  PRIVATE
    gtest
)

target_link_libraries(MiscTests
  PRIVATE
    cloxargs
	  GTest::gtest_main
)

gtest_discover_tests(MiscTests)
