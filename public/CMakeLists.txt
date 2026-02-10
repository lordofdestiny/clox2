add_library(cloxpublic INTERFACE)

file(GLOB_RECURSE TARGET_HEADERS "include/*.h")

target_sources(cloxpublic
  INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
      ${TARGET_HEADERS}
)

if(CLOX_NAN_BOXING)
  target_compile_definitions(cloxpublic INTERFACE NAN_BOXING)
endif()

target_include_directories(
  cloxpublic
  INTERFACE
    include
)
