add_library(cloxargs OBJECT)

target_sources(cloxargs
  PRIVATE
    src/args.c

  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
      include/args.h
)

find_package(argp)
if(NOT ARGP_FOUND)
  add_subdirectory(
    ${PROJECT_SOURCE_DIR}/externals/argp-standalone
    ${CMAKE_BINARY_DIR}/externals/argp-standalone
  )

  target_compile_options(
    argp-standalone
    PRIVATE
      "-Wno-missing-field-initializers"
      "-Wno-unused-parameter"
      "-Wno-pointer-arith"
      "-Wno-pedantic"
      "-Wno-cpp"
  )

  set(ARGP_LIBRARIES argp-standalone)
  target_link_libraries(cloxargs PUBLIC ${ARGP_LIBRARIES})
endif()

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
 