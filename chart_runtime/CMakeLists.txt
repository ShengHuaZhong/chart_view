add_library(chart_runtime SHARED src/chart_runtime_c_api.cpp)
add_library(ChartSystem::runtime ALIAS chart_runtime)

target_link_libraries(
  chart_runtime
  PRIVATE chartsys::options chartsys::warnings)

target_compile_definitions(chart_runtime PRIVATE CHART_RUNTIME_BUILD)

target_include_directories(
  chart_runtime
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/interfaces>
    $<INSTALL_INTERFACE:include>)

set_target_properties(
  chart_runtime
  PROPERTIES OUTPUT_NAME chart_runtime
             EXPORT_NAME runtime
             VERSION ${PROJECT_VERSION}
             SOVERSION ${PROJECT_VERSION_MAJOR})

install(
  TARGETS chart_runtime
  EXPORT ChartSystemTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION include)

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/interfaces/runtime/
  DESTINATION include/runtime
  FILES_MATCHING PATTERN "*.h")
