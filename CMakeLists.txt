cmake_minimum_required(VERSION 3.29)

cmake_policy(SET CMP0155 OLD)

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 23)
endif()
set(CMAKE_CXX_EXTENSIONS OFF)

project(
  ChartSystem
  VERSION 0.1.0
  DESCRIPTION "DLL-first marine chart system build skeleton"
  LANGUAGES CXX C)

include(cmake/PreventInSourceBuilds.cmake)
include(ProjectOptions.cmake)
include(Dependencies.cmake)

option(CHARTSYS_BUILD_RUNTIME "Build chart_runtime shared library" ON)
option(CHARTSYS_BUILD_QTWIDGETS "Build chart_qtwidgets shared library" ON)
option(CHARTSYS_BUILD_STANDALONE "Build chart_standalone executable" ON)
option(CHARTSYS_BUILD_TESTS "Build chart system tests" ${PROJECT_IS_TOP_LEVEL})
option(CHARTSYS_ENABLE_QT_RHI "Enable Qt 6 RHI rendering path" ON)
option(CHARTSYS_ENABLE_S57 "Enable S-57 support scaffolding" ON)
option(CHARTSYS_ENABLE_CM93 "Enable CM93 support scaffolding" OFF)
option(CHARTSYS_ENABLE_S101 "Enable S-101 support scaffolding" OFF)

if(CHARTSYS_BUILD_STANDALONE AND NOT CHARTSYS_BUILD_QTWIDGETS)
  message(FATAL_ERROR "CHARTSYS_BUILD_STANDALONE requires CHARTSYS_BUILD_QTWIDGETS=ON")
endif()

if(CHARTSYS_BUILD_QTWIDGETS AND NOT CHARTSYS_BUILD_RUNTIME)
  message(FATAL_ERROR "CHARTSYS_BUILD_QTWIDGETS requires CHARTSYS_BUILD_RUNTIME=ON")
endif()

chartsys_setup_options()
chartsys_global_options()
chartsys_setup_dependencies()
chartsys_local_options()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if(CHARTSYS_BUILD_QTWIDGETS OR CHARTSYS_BUILD_STANDALONE)
  find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
endif()

if(CHARTSYS_BUILD_RUNTIME)
  add_subdirectory(chart_runtime)
endif()

if(CHARTSYS_BUILD_QTWIDGETS)
  add_subdirectory(chart_qtwidgets)
endif()

if(CHARTSYS_BUILD_STANDALONE)
  add_subdirectory(chart_standalone)
endif()

include(CTest)
if(BUILD_TESTING AND CHARTSYS_BUILD_TESTS)
  add_subdirectory(tests)
endif()

if(CMAKE_SKIP_INSTALL_RULES)
  return()
endif()

include(CMakePackageConfigHelpers)

set(CHARTSYS_PACKAGE_HAS_QT OFF)
if(CHARTSYS_BUILD_QTWIDGETS OR CHARTSYS_BUILD_STANDALONE)
  set(CHARTSYS_PACKAGE_HAS_QT ON)
endif()

install(
  EXPORT ChartSystemTargets
  FILE ChartSystemTargets.cmake
  NAMESPACE ChartSystem::
  DESTINATION lib/cmake/ChartSystem)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ChartSystemConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/ChartSystemConfig.cmake
  INSTALL_DESTINATION lib/cmake/ChartSystem)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/ChartSystemConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ChartSystemConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ChartSystemConfigVersion.cmake
  DESTINATION lib/cmake/ChartSystem)
