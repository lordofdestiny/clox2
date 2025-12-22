add_library(clox_lib SHARED)

file(GLOB_RECURSE clox_lib_SRC "src/*.c")
file(GLOB_RECURSE clox_lib_INC "include/*.h")

target_sources(clox_lib
  PRIVATE
    ${clox_lib_SRC}

  PUBLIC
    FILE_SET HEADERS
    FILES
      ${clox_lib_INC}
)

target_include_directories(clox_lib PUBLIC include)
target_link_libraries(clox_lib PUBLIC m)