add_library(cloximpl SHARED)

file(GLOB_RECURSE cloximpl_SRC "src/*.c")
file(GLOB_RECURSE cloximpl_INC "include/*.h")

target_sources(cloximpl
  PRIVATE
    ${cloximpl_SRC}

  PUBLIC
    FILE_SET HEADERS
    FILES
      ${cloximpl_INC}
)

target_include_directories(cloximpl PUBLIC include)
target_link_libraries(cloximpl PUBLIC m)