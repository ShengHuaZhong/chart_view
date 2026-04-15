include(GenerateExportHeader)

add_library(chart_runtime SHARED
  chart_runtime.cpp
  feature_layer_renderer.cpp
  render_scheduler.cpp
  rhi_render_backend.cpp
  runtime_context.cpp
  cm93/cm93_decode.cpp
  cm93/cm93_reader.cpp
  s57/iso8211.cpp
  s57/s57_reader.cpp
  s101/s101_reader.cpp
  senc/senc_reader.cpp
  senc/senc_writer.cpp)
add_library(chart_view::chart_runtime ALIAS chart_runtime)

target_link_libraries(
  chart_runtime
  PRIVATE chart_view::chart_view_options
          chart_view::chart_view_warnings
          Qt6::GuiPrivate)

target_include_directories(
  chart_runtime
  ${WARNING_GUARD}
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
         $<INSTALL_INTERFACE:include>)

target_compile_features(chart_runtime PUBLIC cxx_std_${CMAKE_CXX_STANDARD})

chart_view_configure_linker(chart_runtime)

set_target_properties(
  chart_runtime
  PROPERTIES VERSION ${PROJECT_VERSION}
             CXX_VISIBILITY_PRESET hidden
             VISIBILITY_INLINES_HIDDEN YES)

generate_export_header(
  chart_runtime
  EXPORT_FILE_NAME "${PROJECT_BINARY_DIR}/include/chart_view/runtime/chart_runtime_export.h"
)

install(
  FILES "${PROJECT_BINARY_DIR}/include/chart_view/runtime/chart_runtime_export.h"
  DESTINATION "include/chart_view/runtime")

install(
  FILES "${PROJECT_SOURCE_DIR}/include/chart_view/runtime/chart_runtime.h"
        "${PROJECT_SOURCE_DIR}/include/chart_view/runtime/chart_runtime_types.h"
        "${PROJECT_SOURCE_DIR}/include/chart_view/runtime/chart_runtime_c_api.h"
  DESTINATION "include/chart_view/runtime")
