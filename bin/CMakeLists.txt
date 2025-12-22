add_executable(clox)

file(GLOB_RECURSE clox_SRC "src/*.c")
file(GLOB_RECURSE clox_INC "include/*.h")

target_sources(clox
  PRIVATE
    ${clox_INC}
    ${clox_SRC}
)

target_include_directories(clox PRIVATE include)
target_link_libraries(clox PRIVATE clox_lib)