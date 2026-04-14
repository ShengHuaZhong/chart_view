include(CMakeDependentOption)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

macro(chartsys_supports_sanitizers)
  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)
    set(TEST_PROGRAM "int main() { return 0; }")

    set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
    set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=undefined")
    check_cxx_source_compiles("${TEST_PROGRAM}" CHARTSYS_HAS_UBSAN_LINK_SUPPORT)

    if(CHARTSYS_HAS_UBSAN_LINK_SUPPORT)
      set(CHARTSYS_SUPPORTS_UBSAN ON)
    else()
      set(CHARTSYS_SUPPORTS_UBSAN OFF)
    endif()
  else()
    set(CHARTSYS_SUPPORTS_UBSAN OFF)
  endif()

  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
    set(CHARTSYS_SUPPORTS_ASAN OFF)
  else()
    if(NOT WIN32)
      set(TEST_PROGRAM "int main() { return 0; }")

      set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
      set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")
      check_cxx_source_compiles("${TEST_PROGRAM}" CHARTSYS_HAS_ASAN_LINK_SUPPORT)

      if(CHARTSYS_HAS_ASAN_LINK_SUPPORT)
        set(CHARTSYS_SUPPORTS_ASAN ON)
      else()
        set(CHARTSYS_SUPPORTS_ASAN OFF)
      endif()
    else()
      set(CHARTSYS_SUPPORTS_ASAN ON)
    endif()
  endif()
endmacro()

macro(chartsys_setup_options)
  option(chartsys_ENABLE_HARDENING "Enable hardening" ON)
  option(chartsys_ENABLE_COVERAGE "Enable coverage reporting" OFF)
  cmake_dependent_option(
    chartsys_ENABLE_GLOBAL_HARDENING
    "Attempt to push hardening options to built dependencies"
    ON
    chartsys_ENABLE_HARDENING
    OFF)

  chartsys_supports_sanitizers()

  if(NOT PROJECT_IS_TOP_LEVEL OR chartsys_PACKAGING_MAINTAINER_MODE)
    option(chartsys_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(chartsys_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(chartsys_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(chartsys_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(chartsys_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(chartsys_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(chartsys_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(chartsys_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(chartsys_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(chartsys_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(chartsys_ENABLE_PCH "Enable precompiled headers" OFF)
    option(chartsys_ENABLE_CACHE "Enable ccache" OFF)
  else()
    option(chartsys_ENABLE_IPO "Enable IPO/LTO" ON)
    option(chartsys_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
    option(chartsys_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${CHARTSYS_SUPPORTS_ASAN})
    option(chartsys_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(chartsys_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${CHARTSYS_SUPPORTS_UBSAN})
    option(chartsys_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(chartsys_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(chartsys_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(chartsys_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(chartsys_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(chartsys_ENABLE_PCH "Enable precompiled headers" OFF)
    option(chartsys_ENABLE_CACHE "Enable ccache" OFF)
  endif()

  if(NOT PROJECT_IS_TOP_LEVEL)
    mark_as_advanced(
      chartsys_ENABLE_IPO
      chartsys_WARNINGS_AS_ERRORS
      chartsys_ENABLE_SANITIZER_ADDRESS
      chartsys_ENABLE_SANITIZER_LEAK
      chartsys_ENABLE_SANITIZER_UNDEFINED
      chartsys_ENABLE_SANITIZER_THREAD
      chartsys_ENABLE_SANITIZER_MEMORY
      chartsys_ENABLE_UNITY_BUILD
      chartsys_ENABLE_CLANG_TIDY
      chartsys_ENABLE_CPPCHECK
      chartsys_ENABLE_COVERAGE
      chartsys_ENABLE_PCH
      chartsys_ENABLE_CACHE)
  endif()
endmacro()

macro(chartsys_global_options)
  if(chartsys_ENABLE_IPO)
    include(cmake/InterproceduralOptimization.cmake)
    myproject_enable_ipo()
  endif()

  chartsys_supports_sanitizers()

  if(chartsys_ENABLE_HARDENING AND chartsys_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT CHARTSYS_SUPPORTS_UBSAN
       OR chartsys_ENABLE_SANITIZER_UNDEFINED
       OR chartsys_ENABLE_SANITIZER_ADDRESS
       OR chartsys_ENABLE_SANITIZER_THREAD
       OR chartsys_ENABLE_SANITIZER_LEAK)
      set(CHARTSYS_ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(CHARTSYS_ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    myproject_enable_hardening(chartsys_options ON ${CHARTSYS_ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()

macro(chartsys_local_options)
  if(PROJECT_IS_TOP_LEVEL)
    include(cmake/StandardProjectSettings.cmake)
  endif()

  add_library(chartsys_warnings INTERFACE)
  add_library(chartsys_options INTERFACE)

  add_library(chartsys::warnings ALIAS chartsys_warnings)
  add_library(chartsys::options ALIAS chartsys_options)

  include(cmake/CompilerWarnings.cmake)
  myproject_set_project_warnings(
    chartsys_warnings
    ${chartsys_WARNINGS_AS_ERRORS}
    ""
    ""
    ""
    "")

  include(cmake/Linker.cmake)

  include(cmake/Sanitizers.cmake)
  myproject_enable_sanitizers(
    chartsys_options
    ${chartsys_ENABLE_SANITIZER_ADDRESS}
    ${chartsys_ENABLE_SANITIZER_LEAK}
    ${chartsys_ENABLE_SANITIZER_UNDEFINED}
    ${chartsys_ENABLE_SANITIZER_THREAD}
    ${chartsys_ENABLE_SANITIZER_MEMORY})

  set_target_properties(chartsys_options PROPERTIES UNITY_BUILD ${chartsys_ENABLE_UNITY_BUILD})

  if(chartsys_ENABLE_PCH)
    target_precompile_headers(
      chartsys_options
      INTERFACE
      <vector>
      <string>
      <utility>)
  endif()

  if(chartsys_ENABLE_CACHE)
    include(cmake/Cache.cmake)
    myproject_enable_cache()
  endif()

  include(cmake/StaticAnalyzers.cmake)
  if(chartsys_ENABLE_CLANG_TIDY)
    myproject_enable_clang_tidy(chartsys_options ${chartsys_WARNINGS_AS_ERRORS})
  endif()

  if(chartsys_ENABLE_CPPCHECK)
    myproject_enable_cppcheck(${chartsys_WARNINGS_AS_ERRORS} "")
  endif()

  if(chartsys_ENABLE_COVERAGE)
    include(cmake/Tests.cmake)
    myproject_enable_coverage(chartsys_options)
  endif()

  if(chartsys_ENABLE_HARDENING AND NOT chartsys_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT CHARTSYS_SUPPORTS_UBSAN
       OR chartsys_ENABLE_SANITIZER_UNDEFINED
       OR chartsys_ENABLE_SANITIZER_ADDRESS
       OR chartsys_ENABLE_SANITIZER_THREAD
       OR chartsys_ENABLE_SANITIZER_LEAK)
      set(CHARTSYS_ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(CHARTSYS_ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    myproject_enable_hardening(chartsys_options OFF ${CHARTSYS_ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()
