find_package(FLEX 2.6.4 REQUIRED)

file(GLOB_RECURSE SCANNER_HEADERS include/*.h)
file(GLOB_RECURSE SCANNER_SOURCES src/*.c)

if(FLEX_FOUND)
  set(SCANNER_SPEC misc/scanner_spec.l)
  set(SCANNER_IMPL_HEADER ${CMAKE_CURRENT_BINARY_DIR}/scanner_impl.h)
  set(SCANNER_IMPL_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/scanner_impl.c)

  flex_target(
    generate_flex_scanner
    ${SCANNER_SPEC}
    ${SCANNER_IMPL_SOURCE}
    DEFINES_FILE ${SCANNER_IMPL_HEADER}
  )

  add_library(cloxscanner_flex OBJECT)

  target_sources(
    cloxscanner_flex
    PRIVATE
      ${SCANNER_SOURCES}
      ${SCANNER_IMPL_SOURCE}
    
    PRIVATE
      FILE_SET private_headers TYPE HEADERS
      BASE_DIRS
        ${CMAKE_CURRENT_BINARY_DIR}
      FILES
        ${SCANNER_IMPL_HEADER}
    
    PUBLIC
      FILE_SET public_headers TYPE HEADERS
      BASE_DIRS
        include
      FILES
        ${SCANNER_HEADERS}
  )

  set_target_properties(
    cloxscanner_flex
    PROPERTIES
      POSITION_INDEPENDENT_CODE TRUE
      C_VISIBILITY_PRESET hidden
      COMPILE_WARNING_AS_ERROR TRUE
  )

  target_include_directories(cloxscanner_flex PUBLIC ${PROJECT_BINARY_DIR}/include)

  target_compile_options(cloxscanner_flex PRIVATE "-Wno-unused-function;-Wno-sign-compare")

  set(COMPILE_DEFS "")
  if(USE_FLEX_SCANNER)
    list(APPEND COMPILE_DEFS USE_FLEX_SCANNER)
  endif()

  list(JOIN COMPILE_DEFS " " COMPILE_DEPS_STRING)
  message(STATUS ${COMPILE_DEPS_STRING})

  target_compile_definitions(
    cloxscanner_flex
    INTERFACE
      "${COMPILE_DEPS_STRING}"
  )

  target_link_libraries(
    cloxscanner_flex
    PUBLIC
      cloxcommon
  )
endif()
