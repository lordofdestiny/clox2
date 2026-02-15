include(GenerateExportHeader)

option(USE_FLEX_SCANNER "Using Flex scanner implementation" NO)
option(CLOX_NAN_BOXING "Enable NAN BOXING for stack values" YES)

add_library(cloximpl SHARED)
file(GLOB_RECURSE TARGET_SOURCES "src/*.c")

add_library(cloximpl_native_api INTERFACE)
if(CLOX_NAN_BOXING)
  target_compile_definitions(cloximpl PUBLIC NAN_BOXING)
  target_compile_definitions(cloximpl_native_api INTERFACE NAN_BOXING)
endif()

add_library(cloximpl::api_native ALIAS cloximpl_native_api)

file(GLOB_RECURSE TARGET_HEADERS_API_NATIVE "public/include/*.h")

target_sources(cloximpl
  PRIVATE
    ${TARGET_SOURCES}

  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
      ${TARGET_HEADERS}
)

generate_export_header(
  cloximpl
  BASE_NAME CLOX
  EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/clox/export.h
)

target_include_directories(cloximpl_native_api INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public/include\;${PROJECT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:${PROJECT_BINARY_DIR}/include>
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

target_compile_features(cloximpl PUBLIC c_std_23)
target_compile_features(cloximpl_native_api INTERFACE c_std_23)

target_link_libraries(cloximpl PUBLIC cloximpl::api_native)

target_link_libraries(cloximpl PRIVATE m)
target_link_libraries(cloximpl PRIVATE cloxcommon)

if(NOT USE_FLEX_SCANNER)
  target_link_libraries(cloximpl PRIVATE cloxscanner_default)
else()
  target_link_libraries(cloximpl PRIVATE cloxscanner_flex)
endif()
