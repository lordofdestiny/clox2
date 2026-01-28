include(FetchContent)

FetchContent_Declare(
  jansson
  GIT_REPOSITORY https://github.com/akheron/jansson
  GIT_TAG        v2.15.0  # Use the latest stable version
)

set(JANSSON_BUILD_DOCS OFF CACHE BOOL "Don't build Jansson docs" FORCE)

FetchContent_MakeAvailable(jansson)
