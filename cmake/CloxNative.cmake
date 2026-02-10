
function(CloxNativeLibrary)
  cmake_parse_arguments(
    CLOX_NATIVE_LIBARARY_PARGS
    ""
    "LIB_NAME;SPEC_FILE;TARGET_NAME;OUT_HEADER;OUT_SOURCE"
    "EXTRA_LIBS"
    "${ARGN}"
  )

  set(LibName ${CLOX_NATIVE_LIBARARY_PARGS_LIB_NAME})
  set(SpecFile ${CLOX_NATIVE_LIBARARY_PARGS_SPEC_FILE})
  set(TargetName ${CLOX_NATIVE_LIBARARY_PARGS_TARGET_NAME})
  set(OutHeader ${CLOX_NATIVE_LIBARARY_PARGS_OUT_HEADER})
  set(OutSource ${CLOX_NATIVE_LIBARARY_PARGS_OUT_SOURCE})
  set(ExtraLibs ${CLOX_NATIVE_LIBARARY_PARGS_EXTRA_LIBS})

  if(NOT CLOX_NATIVE_LIBARARY_PARGS_LIB_NAME)
    message(SEND_ERROR "CLoxNativeLibrary: LIB_NAME not set")
  endif()

  if(NOT CLOX_NATIVE_LIBARARY_PARGS_SPEC_FILE)
    message(STATUS "CLoxNativeLibrary: SPEC_FILE=${CLOX_NATIVE_LIBRARY_PARGS_SPEC_FILE}")
    message(SEND_ERROR "CLoxNativeLibrary: SPEC_FILE not set")
  endif()

  if(NOT CLOX_NATIVE_LIBARARY_PARGS_OUT_HEADER)
    message(SEND_ERROR "CLoxNativeLibrary: OUT_HEADER not set")
  endif()
    
  if(NOT CLOX_NATIVE_LIBARARY_PARGS_OUT_SOURCE)
    message(SEND_ERROR "CLoxNativeLibrary: OUT_SOURCE not set")
  endif()

  if(NOT CLOX_NATIVE_LIBARARY_PARGS_TARGET_NAME)
    set(TargetName "clox${LibName}")
  endif()

  file(GLOB_RECURSE TargetHeaders "include/*.h")
  file(GLOB_RECURSE TargetSources "src/*.c")

  set(IncludeHeader "clox/${LibName}/${LibName}lib.h")
  set(WrapperHeader "${CMAKE_CURRENT_BINARY_DIR}/include/${IncludeHeader}") 
  set(WrapperSource "${CMAKE_CURRENT_BINARY_DIR}/src/${LibName}_wrapper.c")

  cmake_path(GET WrapperHeader PARENT_PATH HeaderDirectory)
  cmake_path(GET WrapperSource PARENT_PATH SourceDirectory)

  file(MAKE_DIRECTORY ${HeaderDirectory} ${SourceDirectory})

  set(${OutHeader} ${WrapperHeader} PARENT_SCOPE)
  set(${OutSource} ${WrapperSource} PARENT_SCOPE)

  if(NOT EXISTS ${PROJECT_SOURCE_DIR}/src) 
    message(SEND_ERROR "CLoxNativeLibrary: ${TargetName} has no 'src' directory")
  endif()
  
  add_custom_command(
    OUTPUT
      ${WrapperHeader}
      ${WrapperSource}
    DEPENDS cloxn ${SpecFile}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND cloxn -p ${SpecFile} ${WrapperHeader} ${WrapperSource} ${IncludeHeader}
  )

  target_sources(
    ${TargetName}
    PRIVATE
      ${TargetSources}
      ${WrapperSource}

    PRIVATE
      FILE_SET HEADERS
      BASE_DIRS
        include
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${TargetHeaders}
      FILES
        ${WrapperHeader}
    )

  add_custom_command(TARGET ${TargetName} POST_BUILD 
    COMMAND "${CMAKE_COMMAND}" -E copy 
      "$<TARGET_FILE:${TargetName}>"
      "${PROJECT_BINARY_DIR}/lib/$<TARGET_FILE_NAME:${TargetName}>" 
    COMMENT "Copying to output directory"
  )

  target_include_directories(
    ${TargetName}
    PRIVATE
      ${PROJECT_BINARY_DIR}/include
      ${CMAKE_CURRENT_BINARY_DIR}/include
  )

  target_compile_options(${TargetName} PRIVATE "-Wno-unused-parameter")

  target_link_libraries(${TargetName} PRIVATE cloxpublic ${ExtraLibs})

endfunction()
