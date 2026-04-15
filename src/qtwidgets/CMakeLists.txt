include(GenerateExportHeader)

add_library(chart_qtwidgets SHARED
  chart_view_widget.cpp
  runtime_bridge.cpp)
add_library(chart_view::chart_qtwidgets ALIAS chart_qtwidgets)

target_link_libraries(
  chart_qtwidgets
  PUBLIC chart_view::chart_runtime
         Qt6::Widgets
  PRIVATE chart_view::chart_view_options
          chart_view::chart_view_warnings)

target_include_directories(
  chart_qtwidgets
  ${WARNING_GUARD}
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
         $<INSTALL_INTERFACE:include>)

target_compile_features(chart_qtwidgets PUBLIC cxx_std_${CMAKE_CXX_STANDARD})

chart_view_configure_linker(chart_qtwidgets)

set_target_properties(
  chart_qtwidgets
  PROPERTIES VERSION ${PROJECT_VERSION}
             CXX_VISIBILITY_PRESET hidden
             VISIBILITY_INLINES_HIDDEN YES)

generate_export_header(
  chart_qtwidgets
  EXPORT_FILE_NAME "${PROJECT_BINARY_DIR}/include/chart_view/qtwidgets/chart_qtwidgets_export.h"
)

install(
  FILES "${PROJECT_BINARY_DIR}/include/chart_view/qtwidgets/chart_qtwidgets_export.h"
  DESTINATION "include/chart_view/qtwidgets")

install(
  FILES "${PROJECT_SOURCE_DIR}/include/chart_view/qtwidgets/chart_view_widget.hpp"
  DESTINATION "include/chart_view/qtwidgets")
