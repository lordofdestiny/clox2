find_package(FLEX 2.6.4 REQUIRED)
find_package(BISON 3.8.2 REQUIRED)

file(GLOB_RECURSE SCANNER_HEADERS include/*.h)
file(GLOB_RECURSE SCANNER_SOURCES src/*.c)

file(GLOB_RECURSE PARSER_HEADERS include/*.h)
file(GLOB_RECURSE PARSER_SOURCES src/*.c)


if(FLEX_FOUND AND BISON_FOUND)
  set(SCANNER_SPEC spec/scanner.l)
  set(SCANNER_IMPL_HEADER ${CMAKE_CURRENT_BINARY_DIR}/include/gen/scanner.h)
  set(SCANNER_IMPL_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/src/gen/scanner.c)

  set(PARSER_SPEC spec/parser.y)
  set(PARSER_IMPL_HEADER ${CMAKE_CURRENT_BINARY_DIR}/include/gen/parser.h)
  set(PARSER_IMPL_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/src/gen/parser.c)

  cmake_path(GET SCANNER_IMPL_HEADER PARENT_PATH SCANNER_INCLUDE_DIR_PATH)
  cmake_path(GET SCANNER_IMPL_SOURCE PARENT_PATH SCANNER_SOURCE_DIR_PATH)
  cmake_path(GET PARSER_IMPL_HEADER PARENT_PATH PARSER_INCLUDE_DIR_PATH)
  cmake_path(GET PARSER_IMPL_SOURCE PARENT_PATH PARSER_SOURCE_DIR_PATH)

  file(MAKE_DIRECTORY
    ${SCANNER_INCLUDE_DIR_PATH}
    ${SCANNER_SOURCE_DIR_PATH}
    ${PARSER_INCLUDE_DIR_PATH}
    ${PARSER_SOURCE_DIR_PATH}
  )

  flex_target(
    generate_flex_scanner
    ${SCANNER_SPEC}
    ${SCANNER_IMPL_SOURCE}
    DEFINES_FILE ${SCANNER_IMPL_HEADER}
  )

  bison_target(
    generate_bison_parser
    ${PARSER_SPEC}
    ${PARSER_IMPL_SOURCE}
    DEFINES_FILE ${PARSER_IMPL_HEADER}
  )

  add_executable(cloxparser)

  target_sources(
    cloxparser
    PRIVATE
      ${SCANNER_SOURCES}
      ${PARSER_SOURCES}
      ${SCANNER_IMPL_SOURCE}
      ${PARSER_IMPL_SOURCE}

    PRIVATE
      FILE_SET private_headers TYPE HEADERS
      BASE_DIRS
        ${CMAKE_CURRENT_BINARY_DIR}/include
      FILES
        ${SCANNER_IMPL_HEADER}
        ${PARSER_IMPL_HEADER}
    
    PUBLIC
      FILE_SET public_headers TYPE HEADERS
      BASE_DIRS
        include
      FILES
        ${SCANNER_HEADERS}
        ${PARSER_HEADERS}
  )

  set_target_properties(
    cloxparser
    PROPERTIES
      POSITION_INDEPENDENT_CODE TRUE
      C_VISIBILITY_PRESET default
      VISIBILITY_INLINES_HIDDEN YES 
      COMPILE_WARNING_AS_ERROR TRUE
  )

  target_include_directories(cloxparser PUBLIC ${PROJECT_BINARY_DIR}/include)

  target_compile_options(cloxparser PRIVATE "-Wno-unused-function;-Wno-sign-compare;-Wno-unused-parameter")

  target_link_libraries(
    cloxparser
    PUBLIC
      cloxcommon
      cloxscanner_common
  )
endif()
