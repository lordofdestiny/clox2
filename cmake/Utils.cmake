macro(IncludeSubdirectories INCLUDE_TESTS)
  file(GLOB V_GLOB LIST_DIRECTORIES true "*")
  foreach(item ${V_GLOB})
    if(IS_DIRECTORY ${item} AND
      (${INCLUDE_TESTS} OR NOT ${item} MATCHES "test"))

        set(V_Yes YES)
        add_subdirectory(${item})
      
    endif()
  endforeach()
endmacro()
