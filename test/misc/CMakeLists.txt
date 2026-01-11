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

add_test(NAME MiscTests COMMAND MiscTests)
