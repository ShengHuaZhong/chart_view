include(GenerateExportHeader)

add_library(chart_runtime SHARED
  area_symbol_renderer.cpp
  chart_runtime.cpp
  catalog/chart_catalog.cpp
  catalog/coverage_index.cpp
  catalog/chart_selection_policy.cpp
  feature_layer_renderer.cpp
  line_symbol_renderer.cpp
  point_symbol_renderer.cpp
  portrayal/display_priority_model.cpp
  portrayal/feature_symbolizer.cpp
  portrayal/opencpn_chartsymbols_parser.cpp
  portrayal/portrayal_registry.cpp
  portrayal/s52_lookup_model.cpp
  portrayal/s52_source_catalog_compiler.cpp
  portrayal/s52_presentation_assets.cpp
  portrayal/s101_rule_table.cpp
  portrayal/s57_rule_table.cpp
  projection/projection_context.cpp
  projection/projected_viewport.cpp
  render_scheduler.cpp
  rhi_render_backend.cpp
  runtime_context.cpp
  text_label_renderer.cpp
  cm93/cm93_decode.cpp
  cm93/cm93_reader.cpp
  quilt/quilt_planner.cpp
  quilt/zoom_policy.cpp
  s57/iso8211.cpp
  s57/s57_reader.cpp
  s57/s57_source_model.cpp
  s57/s57_update_application.cpp
  s101/s101_reader.cpp
  senc/senc_reader.cpp
  senc/senc_writer.cpp)
add_library(chart_view::chart_runtime ALIAS chart_runtime)

target_link_libraries(
  chart_runtime
  PRIVATE chart_view::chart_view_options
          chart_view::chart_view_warnings
          PROJ::proj
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
