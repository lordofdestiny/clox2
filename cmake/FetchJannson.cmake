include(FetchContent)

message(AUTHOR_WARNING "Once new version of jansson merges PR with cmake fixes, move to that tag")
FetchContent_Declare(
  jansson
  GIT_REPOSITORY https://github.com/akheron/jansson
  GIT_TAG        154f8919fb0d3778282b797789ad985e3d99fdc8  # Use the latest stable version
)

set(JANSSON_BUILD_DOCS OFF CACHE BOOL "Don't build Jansson docs" FORCE)

FetchContent_MakeAvailable(jansson)
