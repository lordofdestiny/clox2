function(CloxNativeLibrary)
  cmake_parse_arguments(
    CLOX_NATIVE_LIBARARY_PARGS
    ""
    "SPEC_FILE;TARGET_NAME;OUT_HEADER;OUT_SOURCE"
    "EXTRA_LIBS"
    "${ARGN}"
  )

  set(SpecFile ${CLOX_NATIVE_LIBARARY_PARGS_SPEC_FILE})
  set(TargetName ${CLOX_NATIVE_LIBARARY_PARGS_TARGET_NAME})
  set(OutHeader ${CLOX_NATIVE_LIBARARY_PARGS_OUT_HEADER})
  set(OutSource ${CLOX_NATIVE_LIBARARY_PARGS_OUT_SOURCE})
  set(ExtraLibs ${CLOX_NATIVE_LIBARARY_PARGS_EXTRA_LIBS})

  if(NOT CLOX_NATIVE_LIBARARY_PARGS_SPEC_FILE)
    set(SpecFile "spec.json")
  endif()
  
  cmake_path(
    ABSOLUTE_PATH SpecFile
    BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    NORMALIZE
    OUTPUT_VARIABLE SpecFile
  )

  if(NOT CLOX_NATIVE_LIBARARY_PARGS_TARGET_NAME)
    set(TargetName "clox${LibName}")
  endif()

  if(NOT TargetName MATCHES "clox.*")
    message(FATAL_ERROR "CloxNativeLibrary: TARGET_NAME must start with \"clox\"")
  endif()

  string(REPLACE "clox" "" LibName ${TargetName})

  file(GLOB_RECURSE TargetHeaders "include/*.h")
  file(GLOB_RECURSE TargetSources "src/*.c")

  set(GenIncludePath "${CMAKE_CURRENT_BINARY_DIR}/include")
  set(GenIncludeDir "clox/native/${LibName}")

  set(ExportHeaderInclude "${GenIncludeDir}/${LibName}_export.h")
  set(ExportHeaderPath "${GenIncludePath}/${ExportHeaderInclude}")
  
  set(WrapperHeaderInclude "${GenIncludeDir}/${LibName}.h")
  set(WrapperHeaderPath "${GenIncludePath}/${WrapperHeaderInclude}")

  set(GenSourcePath "${CMAKE_CURRENT_BINARY_DIR}/src")
  set(WrapperSourcePath "${GenSourcePath}/${LibName}_wrapper.c")

  cmake_path(GET WrapperHeaderPath PARENT_PATH HeaderDirectory)
  cmake_path(GET WrapperSourcePath PARENT_PATH SourceDirectory)

  file(MAKE_DIRECTORY ${HeaderDirectory} ${SourceDirectory})

  if(OutHeader)
    set(${OutHeader} ${WrapperHeaderPath} PARENT_SCOPE)
  endif()
  if(OutSource)
    set(${OutSource} ${WrapperSourcePath} PARENT_SCOPE)
  endif()

  if(NOT EXISTS ${PROJECT_SOURCE_DIR}/src) 
    message(SEND_ERROR "CLoxNativeLibrary: ${TargetName} has no 'src' directory")
  endif()

  # TODO Refactor how cloxn -p is called, so that export there is less implicit "knowledge"
  
  add_custom_command(
    OUTPUT
      ${WrapperHeaderPath}
      ${WrapperSourcePath}
    DEPENDS cloxn ${SpecFile} ${ExportHeaderPath}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND cloxn -p ${SpecFile} ${WrapperHeaderPath} ${WrapperSourcePath} ${WrapperHeaderInclude} ${ExportHeaderInclude}
  )

  target_sources(
    ${TargetName}
    PRIVATE
      ${TargetSources}
      ${WrapperSourcePath}

    PRIVATE
      FILE_SET HEADERS
      BASE_DIRS
        include
        ${GenIncludePath}
      FILES
        ${TargetHeaders}
        ${WrapperHeaderPath}
    )

  add_custom_command(TARGET ${TargetName} POST_BUILD 
    COMMAND "${CMAKE_COMMAND}" -E copy 
      "$<TARGET_FILE:${TargetName}>"
      "${PROJECT_BINARY_DIR}/lib/$<TARGET_FILE_NAME:${TargetName}>" 
    COMMENT "Copying native lib ${TargetName} to runpath"
  )

  target_include_directories(
    ${TargetName}
    PRIVATE
      ${PROJECT_BINARY_DIR}/include
      ${CMAKE_CURRENT_BINARY_DIR}/include
  )

  generate_export_header(
    ${TargetName}
      BASE_NAME ""
      EXPORT_FILE_NAME ${ExportHeaderPath}
  )

  target_compile_options(${TargetName} PRIVATE "-Wno-unused-parameter")

  target_link_libraries(${TargetName} PRIVATE cloxpublic ${ExtraLibs})

endfunction()
