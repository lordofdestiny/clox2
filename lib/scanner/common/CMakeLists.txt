add_library(cloxscanner_common INTERFACE)

target_include_directories(
  cloxscanner_common
  INTERFACE
    include
)

add_subdirectory(test)
