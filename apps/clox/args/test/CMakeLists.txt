add_executable(TestCloxArgs)

file(GLOB_RECURSE TARGET_SOURCES "*.c")

target_sources(
  TestCloxArgs
  PRIVATE
    ${TARGET_SOURCES}
)

target_link_libraries(
  TestCloxArgs
  PRIVATE
    cmocka
    cloxargs
)

set_target_properties(
  TestCloxArgs
  PROPERTIES
    COMPILE_WARNING_AS_ERROR TRUE
    INTERPROCEDURAL_OPTIMIZATION TRUE
)

target_compile_features(TestCloxArgs PRIVATE c_std_23)

add_test(NAME TestCloxArgs COMMAND TestCloxArgs)
