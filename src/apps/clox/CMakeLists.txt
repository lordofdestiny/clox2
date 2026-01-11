add_executable(clox)

file(GLOB_RECURSE clox_SRC "*.c") 
file(GLOB_RECURSE clox_INC "include/*.h")

target_sources(clox
  PRIVATE
    ${clox_SRC}

  PRIVATE
  FILE_SET HEADERS
    FILES  
    ${clox_INC}
)

target_include_directories(clox
  PRIVATE
    include
)

target_link_libraries(clox
  PRIVATE
    cloximpl
    cloxargs
)