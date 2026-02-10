include(GenerateExportHeader)

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

target_include_directories(
  cloximpl
  PUBLIC
    ${PROJECT_BINARY_DIR}/include
)

set_target_properties(
  cloximpl
  PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES 
    COMPILE_WARNING_AS_ERROR TRUE
    BUILD_RPATH "${PROJECT_BINARY_DIR}/lib"
)

generate_export_header(
  cloximpl
  BASE_NAME CLOX
  EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/clox_export.h
)

target_link_libraries(cloximpl PRIVATE m)
target_link_libraries(cloximpl PRIVATE cloxcommon)
target_link_libraries(cloximpl PUBLIC cloxpublic)

if(NOT USE_FLEX_SCANNER)
  target_link_libraries(cloximpl PRIVATE cloxscanner)
else()
  target_link_libraries(cloximpl PRIVATE cloxscanner_flex)
endif()
