add_executable(TestArena)

file(GLOB_RECURSE TARGET_SOURCES "*.c")

target_sources(TestArena
  PRIVATE
    ${TARGET_SOURCES}
)

set_target_properties(TestArena
  PROPERTIES
    COMPILE_WARNING_AS_ERROR TRUE
)

target_link_libraries(
  TestArena
  PUBLIC
    cmocka
    cloxcommon
)

add_test(NAME TestArena COMMAND TestArena)
