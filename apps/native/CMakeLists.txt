include(FetchJannson)

add_executable(cloxn)

file(GLOB_RECURSE TARGET_SOURCES "src/*") 

set_target_properties(
  cloxn
  PROPERTIES
    COMPILE_WARNING_AS_ERROR TRUE
)

target_sources(
  cloxn
  
  PRIVATE
    ${TARGET_SOURCES}
)

target_link_libraries(
  cloxn
  
  PRIVATE
    cloxcommon
    jansson::jansson
)
