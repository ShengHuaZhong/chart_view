include(cmake/LibFuzzer.cmake)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)


include(CheckCXXSourceCompiles)


macro(chart_view_supports_sanitizers)
  # Emscripten doesn't support sanitizers
  if(EMSCRIPTEN)
    set(SUPPORTS_UBSAN OFF)
    set(SUPPORTS_ASAN OFF)
  elseif((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)

    message(STATUS "Sanity checking UndefinedBehaviorSanitizer, it should be supported on this platform")
    set(TEST_PROGRAM "int main() { return 0; }")

    # Check if UndefinedBehaviorSanitizer works at link time
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
    set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=undefined")
    check_cxx_source_compiles("${TEST_PROGRAM}" HAS_UBSAN_LINK_SUPPORT)

    if(HAS_UBSAN_LINK_SUPPORT)
      message(STATUS "UndefinedBehaviorSanitizer is supported at both compile and link time.")
      set(SUPPORTS_UBSAN ON)
    else()
      message(WARNING "UndefinedBehaviorSanitizer is NOT supported at link time.")
      set(SUPPORTS_UBSAN OFF)
    endif()
  else()
    set(SUPPORTS_UBSAN OFF)
  endif()

  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
    set(SUPPORTS_ASAN OFF)
  else()
    if (NOT WIN32)
      message(STATUS "Sanity checking AddressSanitizer, it should be supported on this platform")
      set(TEST_PROGRAM "int main() { return 0; }")

      # Check if AddressSanitizer works at link time
      set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
      set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")
      check_cxx_source_compiles("${TEST_PROGRAM}" HAS_ASAN_LINK_SUPPORT)

      if(HAS_ASAN_LINK_SUPPORT)
        message(STATUS "AddressSanitizer is supported at both compile and link time.")
        set(SUPPORTS_ASAN ON)
      else()
        message(WARNING "AddressSanitizer is NOT supported at link time.")
        set(SUPPORTS_ASAN OFF)
      endif()
    else()
      set(SUPPORTS_ASAN ON)
    endif()
  endif()
endmacro()

macro(chart_view_setup_options)
  option(chart_view_ENABLE_HARDENING "Enable hardening" ON)
  option(chart_view_ENABLE_COVERAGE "Enable coverage reporting" OFF)
  option(chart_view_BUILD_RUNTIME "Build chart_runtime shared library" ON)
  option(chart_view_BUILD_QTWIDGETS "Build chart_qtwidgets shared library" ON)
  option(chart_view_BUILD_STANDALONE "Build chart_standalone executable" ON)
  if(PROJECT_IS_TOP_LEVEL)
    option(chart_view_BUILD_TESTS "Build chart system tests" ON)
  else()
    option(chart_view_BUILD_TESTS "Build chart system tests" OFF)
  endif()
  option(chart_view_ENABLE_QT_RHI "Enable Qt 6 RHI rendering path" ON)
  option(chart_view_ENABLE_S57 "Enable S-57 support scaffolding" ON)
  option(chart_view_ENABLE_CM93 "Enable CM93 support scaffolding" OFF)
  option(chart_view_ENABLE_S101 "Enable S-101 support scaffolding" OFF)
  cmake_dependent_option(
    chart_view_ENABLE_GLOBAL_HARDENING
    "Attempt to push hardening options to built dependencies"
    ON
    chart_view_ENABLE_HARDENING
    OFF)

  chart_view_supports_sanitizers()

  set(chart_view_DEFAULT_ASAN ${SUPPORTS_ASAN})
  if(WIN32)
    set(chart_view_DEFAULT_ASAN OFF)
  endif()

  if(NOT PROJECT_IS_TOP_LEVEL OR chart_view_PACKAGING_MAINTAINER_MODE)
    option(chart_view_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(chart_view_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(chart_view_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(chart_view_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(chart_view_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(chart_view_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(chart_view_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(chart_view_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(chart_view_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(chart_view_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(chart_view_ENABLE_PCH "Enable precompiled headers" OFF)
    option(chart_view_ENABLE_CACHE "Enable ccache" OFF)
  else()
    option(chart_view_ENABLE_IPO "Enable IPO/LTO" ON)
    option(chart_view_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
    option(chart_view_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${chart_view_DEFAULT_ASAN})
    option(chart_view_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(chart_view_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
    option(chart_view_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(chart_view_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(chart_view_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(chart_view_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
    option(chart_view_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
    option(chart_view_ENABLE_PCH "Enable precompiled headers" OFF)
    option(chart_view_ENABLE_CACHE "Enable ccache" ON)
  endif()

  if(NOT PROJECT_IS_TOP_LEVEL)
    mark_as_advanced(
      chart_view_ENABLE_IPO
      chart_view_WARNINGS_AS_ERRORS
      chart_view_ENABLE_SANITIZER_ADDRESS
      chart_view_ENABLE_SANITIZER_LEAK
      chart_view_ENABLE_SANITIZER_UNDEFINED
      chart_view_ENABLE_SANITIZER_THREAD
      chart_view_ENABLE_SANITIZER_MEMORY
      chart_view_ENABLE_UNITY_BUILD
      chart_view_ENABLE_CLANG_TIDY
      chart_view_ENABLE_CPPCHECK
      chart_view_ENABLE_COVERAGE
      chart_view_ENABLE_PCH
      chart_view_ENABLE_CACHE
      chart_view_BUILD_RUNTIME
      chart_view_BUILD_QTWIDGETS
      chart_view_BUILD_STANDALONE
      chart_view_BUILD_TESTS
      chart_view_ENABLE_QT_RHI
      chart_view_ENABLE_S57
      chart_view_ENABLE_CM93
      chart_view_ENABLE_S101)
  endif()

  option(chart_view_BUILD_FUZZ_TESTS "Enable fuzz testing executable" OFF)
  mark_as_advanced(chart_view_BUILD_FUZZ_TESTS)

endmacro()

macro(chart_view_global_options)
  if(chart_view_ENABLE_IPO)
    include(cmake/InterproceduralOptimization.cmake)
    chart_view_enable_ipo()
  endif()

  chart_view_supports_sanitizers()

  if(chart_view_ENABLE_HARDENING AND chart_view_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR chart_view_ENABLE_SANITIZER_UNDEFINED
       OR chart_view_ENABLE_SANITIZER_ADDRESS
       OR chart_view_ENABLE_SANITIZER_THREAD
       OR chart_view_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    message("${chart_view_ENABLE_HARDENING} ${ENABLE_UBSAN_MINIMAL_RUNTIME} ${chart_view_ENABLE_SANITIZER_UNDEFINED}")
    chart_view_enable_hardening(chart_view_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()

macro(chart_view_local_options)
  if(PROJECT_IS_TOP_LEVEL)
    include(cmake/StandardProjectSettings.cmake)
  endif()

  add_library(chart_view_warnings INTERFACE)
  add_library(chart_view_options INTERFACE)

  if(MSVC)
    target_compile_options(chart_view_options INTERFACE /EHsc)
  endif()

  include(cmake/CompilerWarnings.cmake)
  chart_view_set_project_warnings(
    chart_view_warnings
    ${chart_view_WARNINGS_AS_ERRORS}
    ""
    ""
    ""
    "")

  include(cmake/Linker.cmake)
  # Must configure each target with linker options, we're avoiding setting it globally for now

  if(NOT EMSCRIPTEN)
    include(cmake/Sanitizers.cmake)
    chart_view_enable_sanitizers(
      chart_view_options
      ${chart_view_ENABLE_SANITIZER_ADDRESS}
      ${chart_view_ENABLE_SANITIZER_LEAK}
      ${chart_view_ENABLE_SANITIZER_UNDEFINED}
      ${chart_view_ENABLE_SANITIZER_THREAD}
      ${chart_view_ENABLE_SANITIZER_MEMORY})
  endif()

  set_target_properties(chart_view_options PROPERTIES UNITY_BUILD ${chart_view_ENABLE_UNITY_BUILD})

  if(chart_view_ENABLE_PCH)
    target_precompile_headers(
      chart_view_options
      INTERFACE
      <vector>
      <string>
      <utility>)
  endif()

  if(chart_view_ENABLE_CACHE)
    include(cmake/Cache.cmake)
    chart_view_enable_cache()
  endif()

  include(cmake/StaticAnalyzers.cmake)
  if(chart_view_ENABLE_CLANG_TIDY)
    chart_view_enable_clang_tidy(chart_view_options ${chart_view_WARNINGS_AS_ERRORS})
  endif()

  if(chart_view_ENABLE_CPPCHECK)
    chart_view_enable_cppcheck(${chart_view_WARNINGS_AS_ERRORS} "" # override cppcheck options
    )
  endif()

  if(chart_view_ENABLE_COVERAGE)
    include(cmake/Tests.cmake)
    chart_view_enable_coverage(chart_view_options)
  endif()

  if(chart_view_WARNINGS_AS_ERRORS)
    check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
    if(LINKER_FATAL_WARNINGS)
      # This is not working consistently, so disabling for now
      # target_link_options(chart_view_options INTERFACE -Wl,--fatal-warnings)
    endif()
  endif()

  if(chart_view_ENABLE_HARDENING AND NOT chart_view_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR chart_view_ENABLE_SANITIZER_UNDEFINED
       OR chart_view_ENABLE_SANITIZER_ADDRESS
       OR chart_view_ENABLE_SANITIZER_THREAD
       OR chart_view_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    chart_view_enable_hardening(chart_view_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()

endmacro()
