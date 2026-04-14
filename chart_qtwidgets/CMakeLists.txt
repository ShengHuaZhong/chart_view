add_library(
  chart_qtwidgets
  SHARED
    src/chart_view_widget.cpp
    src/runtime_bridge.cpp)
add_library(ChartSystem::qtwidgets ALIAS chart_qtwidgets)

target_link_libraries(
  chart_qtwidgets
  PUBLIC ChartSystem::runtime Qt6::Core Qt6::Gui Qt6::Widgets
  PRIVATE chartsys::options chartsys::warnings)

target_compile_definitions(chart_qtwidgets PRIVATE CHART_QTWIDGETS_BUILD)

target_include_directories(
  chart_qtwidgets
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

set_target_properties(
  chart_qtwidgets
  PROPERTIES OUTPUT_NAME chart_qtwidgets
             EXPORT_NAME qtwidgets
             VERSION ${PROJECT_VERSION}
             SOVERSION ${PROJECT_VERSION_MAJOR})

install(
  TARGETS chart_qtwidgets
  EXPORT ChartSystemTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION include)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/chart_qtwidgets/
  DESTINATION include/chart_qtwidgets
  FILES_MATCHING PATTERN "*.hpp")
