add_executable(MiscTests)

file(GLOB_RECURSE TARGET_SOURCES "*.c")

target_sources(
  MiscTests
  PRIVATE
    ${TARGET_SOURCES}
)

target_link_libraries(
  MiscTests
  PRIVATE
    cloxargs
	  cmocka
)

set_target_properties(
  MiscTests
  PROPERTIES
    COMPILE_WARNING_AS_ERROR TRUE
)

target_compile_features(MiscTests PRIVATE c_std_23)

add_test(NAME MiscTests COMMAND MiscTests)
