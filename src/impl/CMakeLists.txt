add_library(cloximpl SHARED)

option(USE_FLEX_SCANNER "Using Flex scanner implementation" NO)

file(GLOB_RECURSE TARGET_SOURCES "src/*.c")
file(GLOB_RECURSE TARGET_HEADERS "include/*.h")

target_sources(cloximpl
  PRIVATE
    ${TARGET_SOURCES}

  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
      ${TARGET_HEADERS}
)

set_target_properties(
  cloximpl
  PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    COMPILE_WARNING_AS_ERROR TRUE
)

target_link_libraries(cloximpl PRIVATE m)
target_link_libraries(cloximpl PRIVATE cloxcommon)

add_subdirectory(scanner)
add_subdirectory(scanner-flex)

if(NOT USE_FLEX_SCANNER)
  target_link_libraries(cloximpl PRIVATE cloxscanner)
else()
  target_link_libraries(cloximpl PRIVATE cloxscanner_flex)
endif()
