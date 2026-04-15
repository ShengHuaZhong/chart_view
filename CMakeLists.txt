cmake_minimum_required(VERSION 3.29)

# Disable modules support, this is broken with clang-tidy at the moment
cmake_policy(SET CMP0155 OLD) 

# This template attempts to be "fetch_content"-able
# so that it works well with tools like CPM or other
# manual dependency management

# Only set the cxx_standard if it is not set by someone else
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 23)
endif()

# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
set(CMAKE_CXX_EXTENSIONS OFF)

# Set the project name and language
project(
  chart_view
  VERSION 0.1.0
  DESCRIPTION "DLL-first marine chart system build skeleton"
  HOMEPAGE_URL "https://github.com/ShengHuaZhong/chart_view"
  LANGUAGES CXX C)

if(MSVC)
  get_filename_component(chart_view_MSVC_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
  get_filename_component(chart_view_MSVC_ROOT_DIR "${chart_view_MSVC_COMPILER_DIR}/../../../../../.." ABSOLUTE)

  file(
    GLOB chart_view_MSVC_DEBUG_RUNTIME_DIRS
    LIST_DIRECTORIES TRUE
    "${chart_view_MSVC_ROOT_DIR}/Redist/MSVC/*/debug_nonredist/x64/Microsoft.VC*.DebugCRT")

  if(chart_view_MSVC_DEBUG_RUNTIME_DIRS)
    list(SORT chart_view_MSVC_DEBUG_RUNTIME_DIRS COMPARE NATURAL ORDER DESCENDING)
    list(GET chart_view_MSVC_DEBUG_RUNTIME_DIRS 0 chart_view_MSVC_RUNTIME_DIR)
  else()
    set(chart_view_MSVC_RUNTIME_DIR "${chart_view_MSVC_COMPILER_DIR}")
  endif()

  set(chart_view_MSVC_RUNTIME_DIR "${chart_view_MSVC_RUNTIME_DIR}" CACHE INTERNAL "MSVC runtime directory" FORCE)
endif()

include(cmake/PreventInSourceBuilds.cmake)
include(cmake/Utilities.cmake)
include(ProjectOptions.cmake)


chart_view_setup_options()

if(chart_view_BUILD_QTWIDGETS AND NOT chart_view_BUILD_RUNTIME)
  message(FATAL_ERROR "chart_view_BUILD_QTWIDGETS requires chart_view_BUILD_RUNTIME")
endif()

if(chart_view_BUILD_STANDALONE AND NOT chart_view_BUILD_QTWIDGETS)
  message(FATAL_ERROR "chart_view_BUILD_STANDALONE requires chart_view_BUILD_QTWIDGETS")
endif()

include(CTest)

chart_view_global_options()
include(Dependencies.cmake)
chart_view_setup_dependencies()

chart_view_local_options()

option(CHARTSYS_ENABLE_REAL_CHART_TESTS "Enable real local chart-data integration tests" OFF)

set(CHARTSYS_S57_TESTDATA_ROOT "" CACHE PATH "Path to local S57 chart test data")
set(CHARTSYS_CM93_TESTDATA_ROOT "" CACHE PATH "Path to local CM93 chart test data")

if(CHARTSYS_ENABLE_REAL_CHART_TESTS)
  if(CHARTSYS_S57_TESTDATA_ROOT STREQUAL "" OR NOT EXISTS "${CHARTSYS_S57_TESTDATA_ROOT}")
    message(FATAL_ERROR "CHARTSYS_S57_TESTDATA_ROOT is not set or does not exist: '${CHARTSYS_S57_TESTDATA_ROOT}'")
  endif()
  if(CHARTSYS_CM93_TESTDATA_ROOT STREQUAL "" OR NOT EXISTS "${CHARTSYS_CM93_TESTDATA_ROOT}")
    message(FATAL_ERROR "CHARTSYS_CM93_TESTDATA_ROOT is not set or does not exist: '${CHARTSYS_CM93_TESTDATA_ROOT}'")
  endif()
  message(STATUS "Real chart tests enabled:")
  message(STATUS "  S57 root:  ${CHARTSYS_S57_TESTDATA_ROOT}")
  message(STATUS "  CM93 root: ${CHARTSYS_CM93_TESTDATA_ROOT}")
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(GIT_SHA
    "Unknown"
    CACHE STRING "SHA this build was generated from")
string(
  SUBSTRING "${GIT_SHA}"
            0
            8
            GIT_SHORT_SHA)

target_compile_features(chart_view_options INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

add_library(chart_view::chart_view_options ALIAS chart_view_options)
add_library(chart_view::chart_view_warnings ALIAS chart_view_warnings)

add_subdirectory(configured_files)

if(chart_view_BUILD_RUNTIME)
  add_subdirectory(src/runtime)
endif()

if(chart_view_BUILD_QTWIDGETS)
  add_subdirectory(src/qtwidgets)
endif()

if(chart_view_BUILD_STANDALONE)
  add_subdirectory(apps/chart_standalone)
endif()

if(NOT PROJECT_IS_TOP_LEVEL)
  return()
endif()

if(BUILD_TESTING AND chart_view_BUILD_TESTS)
  add_subdirectory(test)
endif()

if(MSVC)
  get_all_installable_targets(all_targets)
  if(all_targets)
    set_target_properties(${all_targets} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=$(VC_ExecutablePath_x64);%PATH%")
  endif()
endif()

if(TARGET chart_standalone)
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT chart_standalone)
endif()

if(CMAKE_SKIP_INSTALL_RULES)
  return()
endif()

include(cmake/PackageProject.cmake)

set(chart_view_package_targets)
set(chart_view_public_includes include)

if(TARGET chart_runtime)
  list(APPEND chart_view_package_targets chart_runtime)
endif()

if(TARGET chart_qtwidgets)
  list(APPEND chart_view_package_targets chart_qtwidgets)
endif()

if(TARGET chart_standalone)
  list(APPEND chart_view_package_targets chart_standalone)
endif()

set(chart_view_public_dependencies)
if(TARGET chart_qtwidgets)
  list(APPEND chart_view_public_dependencies "Qt6 COMPONENTS Core Gui GuiPrivate Widgets")
endif()

chart_view_package_project(
  TARGETS
  ${chart_view_package_targets}
  PUBLIC_INCLUDES
  ${chart_view_public_includes}
  PUBLIC_DEPENDENCIES
  ${chart_view_public_dependencies}
)

chart_view_install_runtime_support_dlls()

set(CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${GIT_SHORT_SHA}-${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
)

include(CPack)
