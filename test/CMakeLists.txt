function(chart_view_prepend_path path_var)
  foreach(_chart_view_path_entry IN LISTS ARGN)
    if(NOT _chart_view_path_entry STREQUAL "")
      set(${path_var} "${_chart_view_path_entry};${${path_var}}")
    endif()
  endforeach()
  set(${path_var} "${${path_var}}" PARENT_SCOPE)
endfunction()

function(chart_view_add_cmake_script_test test_name)
  cmake_parse_arguments(
    PARSE_ARGV
    1
    CHART_VIEW_TEST
    ""
    "COMMAND;PATH_DEBUG;PATH_RELEASE;QT_PLUGIN_PATH;QT_QPA_PLATFORM"
    "ARGS")

  set(_chart_view_test_path "$<$<CONFIG:Debug>:${CHART_VIEW_TEST_PATH_DEBUG}>$<$<NOT:$<CONFIG:Debug>>:${CHART_VIEW_TEST_PATH_RELEASE}>")
  set(_chart_view_script_path "${CMAKE_CURRENT_BINARY_DIR}/run_${test_name}_$<CONFIG>.cmake")
  string(REPLACE "." "_" _chart_view_status_id "${test_name}")

  set(_chart_view_script_content
      "set(ENV{PATH} [==[${_chart_view_test_path}]==])\n")

  if(NOT CHART_VIEW_TEST_QT_PLUGIN_PATH STREQUAL "")
    string(
      APPEND
      _chart_view_script_content
      "set(ENV{QT_PLUGIN_PATH} [==[${CHART_VIEW_TEST_QT_PLUGIN_PATH}]==])\n")
  endif()

  if(NOT CHART_VIEW_TEST_QT_QPA_PLATFORM STREQUAL "")
    string(
      APPEND
      _chart_view_script_content
      "set(ENV{QT_QPA_PLATFORM} [==[${CHART_VIEW_TEST_QT_QPA_PLATFORM}]==])\n")
  endif()

  string(
    APPEND
    _chart_view_script_content
    "execute_process(\n"
    "  COMMAND [==[${CHART_VIEW_TEST_COMMAND}]==]")

  foreach(_chart_view_arg IN LISTS CHART_VIEW_TEST_ARGS)
    string(APPEND _chart_view_script_content " [==[${_chart_view_arg}]==]")
  endforeach()

  string(
    APPEND
    _chart_view_script_content
    "\n"
    "  COMMAND_ECHO STDOUT\n"
    "  RESULT_VARIABLE chart_view_test_result)\n"
    "if(NOT chart_view_test_result EQUAL 0)\n"
    "  message(FATAL_ERROR \"${_chart_view_status_id} failed with exit code ${chart_view_test_result}\")\n"
    "endif()\n")

  file(GENERATE OUTPUT "${_chart_view_script_path}" CONTENT "${_chart_view_script_content}")

  add_test(NAME "${test_name}" COMMAND ${CMAKE_COMMAND} -P "${_chart_view_script_path}")
endfunction()

set(chart_view_runtime_test_path_debug "$ENV{PATH}")
set(chart_view_runtime_test_path_release "$ENV{PATH}")

if(chart_view_MSVC_RUNTIME_DIR)
  chart_view_prepend_path(chart_view_runtime_test_path_debug "${chart_view_MSVC_RUNTIME_DIR}")
  chart_view_prepend_path(chart_view_runtime_test_path_release "${chart_view_MSVC_RUNTIME_DIR}")
endif()

if(chart_view_VCPKG_BIN_DIR)
  chart_view_prepend_path(chart_view_runtime_test_path_debug "${chart_view_VCPKG_BIN_DIR}")
  chart_view_prepend_path(chart_view_runtime_test_path_release "${chart_view_VCPKG_BIN_DIR}")
endif()

if(chart_view_VCPKG_DEBUG_BIN_DIR)
  chart_view_prepend_path(chart_view_runtime_test_path_debug "${chart_view_VCPKG_DEBUG_BIN_DIR}")
endif()

set(chart_view_qt_test_path_debug "${chart_view_runtime_test_path_debug}")
set(chart_view_qt_test_path_release "${chart_view_runtime_test_path_release}")

if(chart_view_QT_BIN_DIR)
  chart_view_prepend_path(chart_view_qt_test_path_debug "${chart_view_QT_BIN_DIR}")
  chart_view_prepend_path(chart_view_qt_test_path_release "${chart_view_QT_BIN_DIR}")
endif()

add_executable(runtime_api_tests
  runtime/runtime_api_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp")
target_link_libraries(
  runtime_api_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(runtime_api_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(runtime_api_tests)
chart_view_copy_runtime_dlls(runtime_api_tests)

chart_view_add_cmake_script_test(
  runtime.api
  COMMAND "$<TARGET_FILE:runtime_api_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(viewport_scene_tests
  runtime/viewport_scene_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/render_scheduler.cpp")
target_link_libraries(
  viewport_scene_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(viewport_scene_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(viewport_scene_tests)
chart_view_copy_runtime_dlls(viewport_scene_tests)

chart_view_add_cmake_script_test(
  runtime.viewport_scene
  COMMAND "$<TARGET_FILE:viewport_scene_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(projection_context_tests
  runtime/projection_context_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projection_context.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projected_viewport.cpp")
target_link_libraries(
  projection_context_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          PROJ::proj
          Catch2::Catch2WithMain)
target_include_directories(projection_context_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(projection_context_tests)
chart_view_copy_runtime_dlls(projection_context_tests)

chart_view_add_cmake_script_test(
  runtime.projection_context
  COMMAND "$<TARGET_FILE:projection_context_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(rhi_render_tests
  runtime/rhi_render_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/rhi_render_backend.cpp")
target_link_libraries(
  rhi_render_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Qt6::GuiPrivate
          Catch2::Catch2WithMain)
target_include_directories(rhi_render_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(rhi_render_tests)
chart_view_copy_runtime_dlls(rhi_render_tests)

chart_view_add_cmake_script_test(
  runtime.rhi_render
  COMMAND "$<TARGET_FILE:rhi_render_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}"
  QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
  QT_QPA_PLATFORM "offscreen")

# ── Feature layer renderer ────────────────────────────────────────
add_executable(feature_renderer_tests
  runtime/feature_renderer_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/area_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/feature_layer_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/line_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/point_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projection_context.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projected_viewport.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/text_label_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/display_priority_model.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/feature_symbolizer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/portrayal_registry.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s52_presentation_assets.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s101_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s57_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/rhi_render_backend.cpp")
target_link_libraries(
  feature_renderer_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          PROJ::proj
          Qt6::GuiPrivate
          Catch2::Catch2WithMain)
target_include_directories(feature_renderer_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(feature_renderer_tests)
chart_view_copy_runtime_dlls(feature_renderer_tests)

chart_view_add_cmake_script_test(
  runtime.feature_renderer
  COMMAND "$<TARGET_FILE:feature_renderer_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}"
  QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
  QT_QPA_PLATFORM "offscreen")

add_executable(display_priority_tests
  runtime/display_priority_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/display_priority_model.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s101_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s57_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/feature_symbolizer.cpp")
target_link_libraries(
  display_priority_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(display_priority_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(display_priority_tests)
chart_view_copy_runtime_dlls(display_priority_tests)

chart_view_add_cmake_script_test(
  runtime.display_priority
  COMMAND "$<TARGET_FILE:display_priority_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(point_symbol_tests
  runtime/point_symbol_renderer_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/point_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/rhi_render_backend.cpp")
target_link_libraries(
  point_symbol_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Qt6::GuiPrivate
          Catch2::Catch2WithMain)
target_include_directories(point_symbol_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(point_symbol_tests)
chart_view_copy_runtime_dlls(point_symbol_tests)

chart_view_add_cmake_script_test(
  runtime.point_symbol
  COMMAND "$<TARGET_FILE:point_symbol_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}"
  QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
  QT_QPA_PLATFORM "offscreen")

add_executable(line_symbol_tests
  runtime/line_symbol_renderer_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/line_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/rhi_render_backend.cpp")
target_link_libraries(
  line_symbol_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Qt6::GuiPrivate
          Catch2::Catch2WithMain)
target_include_directories(line_symbol_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(line_symbol_tests)
chart_view_copy_runtime_dlls(line_symbol_tests)

chart_view_add_cmake_script_test(
  runtime.line_symbol
  COMMAND "$<TARGET_FILE:line_symbol_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}"
  QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
  QT_QPA_PLATFORM "offscreen")

add_executable(area_symbol_tests
  runtime/area_symbol_renderer_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/area_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/rhi_render_backend.cpp")
target_link_libraries(
  area_symbol_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Qt6::GuiPrivate
          Catch2::Catch2WithMain)
target_include_directories(area_symbol_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(area_symbol_tests)
chart_view_copy_runtime_dlls(area_symbol_tests)

chart_view_add_cmake_script_test(
  runtime.area_symbol
  COMMAND "$<TARGET_FILE:area_symbol_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}"
  QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
  QT_QPA_PLATFORM "offscreen")

add_executable(label_tests
  runtime/text_label_renderer_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projection_context.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projected_viewport.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/text_label_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/rhi_render_backend.cpp")
target_link_libraries(
  label_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          PROJ::proj
          Qt6::GuiPrivate
          Catch2::Catch2WithMain)
target_include_directories(label_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(label_tests)
chart_view_copy_runtime_dlls(label_tests)

chart_view_add_cmake_script_test(
  runtime.label
  COMMAND "$<TARGET_FILE:label_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}"
  QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
  QT_QPA_PLATFORM "offscreen")

add_executable(unicode_text_tests runtime/unicode_text_tests.cpp)
target_link_libraries(
  unicode_text_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(unicode_text_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(unicode_text_tests)
chart_view_copy_runtime_dlls(unicode_text_tests)

chart_view_add_cmake_script_test(
  runtime.unicode_text
  COMMAND "$<TARGET_FILE:unicode_text_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(font_fallback_tests runtime/font_fallback_tests.cpp)
target_link_libraries(
  font_fallback_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Qt6::Gui
          Catch2::Catch2WithMain)
target_include_directories(font_fallback_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(font_fallback_tests)
chart_view_copy_runtime_dlls(font_fallback_tests)

chart_view_add_cmake_script_test(
  runtime.font_fallback
  COMMAND "$<TARGET_FILE:font_fallback_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}"
  QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
  QT_QPA_PLATFORM "offscreen")

add_executable(glyph_cache_tests runtime/glyph_cache_tests.cpp)
target_link_libraries(
  glyph_cache_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Qt6::Gui
          Catch2::Catch2WithMain)
target_include_directories(glyph_cache_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(glyph_cache_tests)
chart_view_copy_runtime_dlls(glyph_cache_tests)

chart_view_add_cmake_script_test(
  runtime.glyph_cache
  COMMAND "$<TARGET_FILE:glyph_cache_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}"
  QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
  QT_QPA_PLATFORM "offscreen")

add_executable(feature_symbolizer_tests
  runtime/feature_symbolizer_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/feature_symbolizer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/portrayal_registry.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s52_presentation_assets.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s101_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s57_rule_table.cpp")
target_link_libraries(
  feature_symbolizer_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(feature_symbolizer_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(feature_symbolizer_tests)
chart_view_copy_runtime_dlls(feature_symbolizer_tests)

chart_view_add_cmake_script_test(
  runtime.feature_symbolizer
  COMMAND "$<TARGET_FILE:feature_symbolizer_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(s64_reference_smoke_tests
  runtime/s64_reference_smoke_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/area_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/feature_layer_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/line_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/point_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projection_context.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projected_viewport.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/text_label_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/display_priority_model.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/feature_symbolizer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/portrayal_registry.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s52_presentation_assets.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s101_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s57_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/rhi_render_backend.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp")
target_link_libraries(
  s64_reference_smoke_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          PROJ::proj
          Qt6::GuiPrivate
          Catch2::Catch2WithMain)
target_include_directories(s64_reference_smoke_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(s64_reference_smoke_tests)
chart_view_copy_runtime_dlls(s64_reference_smoke_tests)

chart_view_add_cmake_script_test(
  runtime.s64_reference_smoke
  COMMAND "$<TARGET_FILE:s64_reference_smoke_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}"
  QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
  QT_QPA_PLATFORM "offscreen")

add_executable(s57_rule_table_tests
  runtime/s57_rule_table_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s57_rule_table.cpp")
target_link_libraries(
  s57_rule_table_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(s57_rule_table_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(s57_rule_table_tests)
chart_view_copy_runtime_dlls(s57_rule_table_tests)

chart_view_add_cmake_script_test(
  runtime.s57_rule_table
  COMMAND "$<TARGET_FILE:s57_rule_table_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(s101_rule_table_tests
  runtime/s101_rule_table_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s101_rule_table.cpp")
target_link_libraries(
  s101_rule_table_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(s101_rule_table_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(s101_rule_table_tests)
chart_view_copy_runtime_dlls(s101_rule_table_tests)

chart_view_add_cmake_script_test(
  runtime.s101_rule_table
  COMMAND "$<TARGET_FILE:s101_rule_table_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(portrayal_registry_tests
  runtime/portrayal_registry_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/portrayal_registry.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s52_presentation_assets.cpp")
target_link_libraries(
  portrayal_registry_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(portrayal_registry_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(portrayal_registry_tests)
chart_view_copy_runtime_dlls(portrayal_registry_tests)

chart_view_add_cmake_script_test(
  runtime.portrayal_registry
  COMMAND "$<TARGET_FILE:portrayal_registry_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(s52_presentation_assets_tests
  runtime/s52_presentation_assets_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s52_presentation_assets.cpp")
target_link_libraries(
  s52_presentation_assets_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(s52_presentation_assets_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(s52_presentation_assets_tests)
chart_view_copy_runtime_dlls(s52_presentation_assets_tests)

chart_view_add_cmake_script_test(
  runtime.s52_presentation_assets
  COMMAND "$<TARGET_FILE:s52_presentation_assets_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(s52_lookup_model_tests runtime/s52_lookup_model_tests.cpp)
target_link_libraries(
  s52_lookup_model_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(s52_lookup_model_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(s52_lookup_model_tests)
chart_view_copy_runtime_dlls(s52_lookup_model_tests)

chart_view_add_cmake_script_test(
  runtime.s52_lookup_model
  COMMAND "$<TARGET_FILE:s52_lookup_model_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(s52_display_settings_tests runtime/s52_display_settings_tests.cpp)
target_link_libraries(
  s52_display_settings_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(s52_display_settings_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(s52_display_settings_tests)
chart_view_copy_runtime_dlls(s52_display_settings_tests)

chart_view_add_cmake_script_test(
  runtime.s52_display_settings
  COMMAND "$<TARGET_FILE:s52_display_settings_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(s52_conditional_symbology_tests runtime/s52_conditional_symbology_tests.cpp)
target_link_libraries(
  s52_conditional_symbology_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(s52_conditional_symbology_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(s52_conditional_symbology_tests)
chart_view_copy_runtime_dlls(s52_conditional_symbology_tests)

chart_view_add_cmake_script_test(
  runtime.s52_conditional_symbology
  COMMAND "$<TARGET_FILE:s52_conditional_symbology_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(feature_model_tests runtime/feature_model_tests.cpp)
target_link_libraries(
  feature_model_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(feature_model_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(feature_model_tests)
chart_view_copy_runtime_dlls(feature_model_tests)

chart_view_add_cmake_script_test(
  runtime.feature_model
  COMMAND "$<TARGET_FILE:feature_model_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(senc_section_tests runtime/senc_section_tests.cpp)
target_link_libraries(
  senc_section_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(senc_section_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(senc_section_tests)
chart_view_copy_runtime_dlls(senc_section_tests)

chart_view_add_cmake_script_test(
  runtime.senc_section
  COMMAND "$<TARGET_FILE:senc_section_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(senc_writer_tests
  runtime/senc_writer_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp")
target_link_libraries(
  senc_writer_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(senc_writer_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(senc_writer_tests)
chart_view_copy_runtime_dlls(senc_writer_tests)

chart_view_add_cmake_script_test(
  runtime.senc_writer
  COMMAND "$<TARGET_FILE:senc_writer_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(senc_reader_tests
  runtime/senc_reader_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp")
target_link_libraries(
  senc_reader_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(senc_reader_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(senc_reader_tests)
chart_view_copy_runtime_dlls(senc_reader_tests)

chart_view_add_cmake_script_test(
  runtime.senc_reader
  COMMAND "$<TARGET_FILE:senc_reader_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(senc_v2_tests
  runtime/senc_v2_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp")
target_link_libraries(
  senc_v2_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(senc_v2_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(senc_v2_tests)
chart_view_copy_runtime_dlls(senc_v2_tests)

chart_view_add_cmake_script_test(
  runtime.senc_v2
  COMMAND "$<TARGET_FILE:senc_v2_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(senc_validator_tests
  runtime/senc_validator_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_validator.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp")
target_link_libraries(
  senc_validator_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(senc_validator_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(senc_validator_tests)
chart_view_copy_runtime_dlls(senc_validator_tests)

chart_view_add_cmake_script_test(
  runtime.senc_validator
  COMMAND "$<TARGET_FILE:senc_validator_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(s57_reader_tests
  runtime/s57_reader_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/s57/iso8211.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/s57/s57_reader.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/s57/s57_source_model.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/s57/s57_update_application.cpp")
target_link_libraries(
  s57_reader_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(s57_reader_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

if(CHARTSYS_ENABLE_REAL_CHART_TESTS AND NOT CHARTSYS_S57_TESTDATA_ROOT STREQUAL "")
  target_compile_definitions(s57_reader_tests PRIVATE
    "CHARTSYS_S57_TESTDATA_ROOT=\"${CHARTSYS_S57_TESTDATA_ROOT}\"")
endif()

chart_view_configure_linker(s57_reader_tests)
chart_view_copy_runtime_dlls(s57_reader_tests)

chart_view_add_cmake_script_test(
  runtime.s57_reader
  COMMAND "$<TARGET_FILE:s57_reader_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(cm93_reader_tests
  runtime/cm93_reader_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/cm93/cm93_decode.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/cm93/cm93_reader.cpp")
target_link_libraries(
  cm93_reader_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(cm93_reader_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

if(CHARTSYS_ENABLE_REAL_CHART_TESTS AND NOT CHARTSYS_CM93_TESTDATA_ROOT STREQUAL "")
  target_compile_definitions(cm93_reader_tests PRIVATE
    "CHARTSYS_CM93_TESTDATA_ROOT=\"${CHARTSYS_CM93_TESTDATA_ROOT}\"")
endif()

chart_view_configure_linker(cm93_reader_tests)
chart_view_copy_runtime_dlls(cm93_reader_tests)

chart_view_add_cmake_script_test(
  runtime.cm93_reader
  COMMAND "$<TARGET_FILE:cm93_reader_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(s101_reader_tests
  runtime/s101_reader_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/s101/s101_reader.cpp")
target_link_libraries(
  s101_reader_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(s101_reader_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(s101_reader_tests)
chart_view_copy_runtime_dlls(s101_reader_tests)

chart_view_add_cmake_script_test(
  runtime.s101_reader
  COMMAND "$<TARGET_FILE:s101_reader_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(s57_senc_smoke_tests
  runtime/s57_senc_smoke_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/s57/iso8211.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/s57/s57_reader.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/s57/s57_source_model.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/s57/s57_update_application.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp")
target_link_libraries(
  s57_senc_smoke_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(s57_senc_smoke_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

if(CHARTSYS_ENABLE_REAL_CHART_TESTS AND NOT CHARTSYS_S57_TESTDATA_ROOT STREQUAL "")
  target_compile_definitions(s57_senc_smoke_tests PRIVATE
    "CHARTSYS_S57_TESTDATA_ROOT=\"${CHARTSYS_S57_TESTDATA_ROOT}\"")
endif()

chart_view_configure_linker(s57_senc_smoke_tests)
chart_view_copy_runtime_dlls(s57_senc_smoke_tests)

chart_view_add_cmake_script_test(
  runtime.s57_senc_smoke
  COMMAND "$<TARGET_FILE:s57_senc_smoke_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(s57_quilt_smoke_tests
  runtime/s57_quilt_smoke_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/area_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/catalog/chart_catalog.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/catalog/coverage_index.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/catalog/chart_selection_policy.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/feature_layer_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/line_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/point_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projection_context.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projected_viewport.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/text_label_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/display_priority_model.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/feature_symbolizer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/portrayal_registry.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s52_presentation_assets.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s101_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s57_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/quilt/quilt_planner.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/quilt/zoom_policy.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/rhi_render_backend.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/s57/iso8211.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/s57/s57_reader.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp")
target_link_libraries(
  s57_quilt_smoke_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          PROJ::proj
          Qt6::GuiPrivate
          Catch2::Catch2WithMain)
target_include_directories(s57_quilt_smoke_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

if(CHARTSYS_ENABLE_REAL_CHART_TESTS AND NOT CHARTSYS_S57_TESTDATA_ROOT STREQUAL "")
  target_compile_definitions(s57_quilt_smoke_tests PRIVATE
    "CHARTSYS_S57_TESTDATA_ROOT=\"${CHARTSYS_S57_TESTDATA_ROOT}\"")
endif()

chart_view_configure_linker(s57_quilt_smoke_tests)
chart_view_copy_runtime_dlls(s57_quilt_smoke_tests)

chart_view_add_cmake_script_test(
  runtime.s57_quilt_smoke
  COMMAND "$<TARGET_FILE:s57_quilt_smoke_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}"
  QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
  QT_QPA_PLATFORM "offscreen")

add_executable(s57_symbolized_smoke_tests
  runtime/s57_symbolized_smoke_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/area_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/feature_layer_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/line_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/point_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projection_context.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projected_viewport.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/text_label_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/display_priority_model.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/feature_symbolizer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/portrayal_registry.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s52_presentation_assets.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s101_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s57_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/rhi_render_backend.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp")
target_link_libraries(
  s57_symbolized_smoke_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          PROJ::proj
          Qt6::GuiPrivate
          Catch2::Catch2WithMain)
target_include_directories(s57_symbolized_smoke_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(s57_symbolized_smoke_tests)
chart_view_copy_runtime_dlls(s57_symbolized_smoke_tests)

chart_view_add_cmake_script_test(
  runtime.s57_symbolized_smoke
  COMMAND "$<TARGET_FILE:s57_symbolized_smoke_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}"
  QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
  QT_QPA_PLATFORM "offscreen")

add_executable(cm93_senc_smoke_tests
  runtime/cm93_senc_smoke_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/cm93/cm93_decode.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/cm93/cm93_reader.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp")
target_link_libraries(
  cm93_senc_smoke_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(cm93_senc_smoke_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

if(CHARTSYS_ENABLE_REAL_CHART_TESTS AND NOT CHARTSYS_CM93_TESTDATA_ROOT STREQUAL "")
  target_compile_definitions(cm93_senc_smoke_tests PRIVATE
    "CHARTSYS_CM93_TESTDATA_ROOT=\"${CHARTSYS_CM93_TESTDATA_ROOT}\"")
endif()

chart_view_configure_linker(cm93_senc_smoke_tests)
chart_view_copy_runtime_dlls(cm93_senc_smoke_tests)

chart_view_add_cmake_script_test(
  runtime.cm93_senc_smoke
  COMMAND "$<TARGET_FILE:cm93_senc_smoke_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(cm93_quilt_smoke_tests
  runtime/cm93_quilt_smoke_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/area_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/catalog/chart_catalog.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/catalog/coverage_index.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/catalog/chart_selection_policy.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/cm93/cm93_decode.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/cm93/cm93_reader.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/feature_layer_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/line_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/point_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projection_context.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projected_viewport.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/text_label_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/display_priority_model.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/feature_symbolizer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/portrayal_registry.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s52_presentation_assets.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s101_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s57_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/quilt/quilt_planner.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/quilt/zoom_policy.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/rhi_render_backend.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp")
target_link_libraries(
  cm93_quilt_smoke_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          PROJ::proj
          Qt6::GuiPrivate
          Catch2::Catch2WithMain)
target_include_directories(cm93_quilt_smoke_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

if(CHARTSYS_ENABLE_REAL_CHART_TESTS AND NOT CHARTSYS_CM93_TESTDATA_ROOT STREQUAL "")
  target_compile_definitions(cm93_quilt_smoke_tests PRIVATE
    "CHARTSYS_CM93_TESTDATA_ROOT=\"${CHARTSYS_CM93_TESTDATA_ROOT}\"")
endif()

chart_view_configure_linker(cm93_quilt_smoke_tests)
chart_view_copy_runtime_dlls(cm93_quilt_smoke_tests)

chart_view_add_cmake_script_test(
  runtime.cm93_quilt_smoke
  COMMAND "$<TARGET_FILE:cm93_quilt_smoke_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}"
  QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
  QT_QPA_PLATFORM "offscreen")

# ── S-101 SENC smoke ──────────────────────────────────────────────
add_executable(s101_senc_smoke_tests
  runtime/s101_senc_smoke_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/s101/s101_reader.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp")
target_link_libraries(
  s101_senc_smoke_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(s101_senc_smoke_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(s101_senc_smoke_tests)
chart_view_copy_runtime_dlls(s101_senc_smoke_tests)

chart_view_add_cmake_script_test(
  runtime.s101_senc_smoke
  COMMAND "$<TARGET_FILE:s101_senc_smoke_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(s101_quilt_smoke_tests
  runtime/s101_quilt_smoke_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/area_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/catalog/chart_catalog.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/catalog/coverage_index.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/catalog/chart_selection_policy.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/feature_layer_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/line_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/point_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projection_context.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projected_viewport.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/text_label_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/display_priority_model.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/feature_symbolizer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/portrayal_registry.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s52_presentation_assets.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s101_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s57_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/quilt/quilt_planner.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/quilt/zoom_policy.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/rhi_render_backend.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/s101/s101_reader.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp")
target_link_libraries(
  s101_quilt_smoke_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          PROJ::proj
          Qt6::GuiPrivate
          Catch2::Catch2WithMain)
target_include_directories(s101_quilt_smoke_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(s101_quilt_smoke_tests)
chart_view_copy_runtime_dlls(s101_quilt_smoke_tests)

chart_view_add_cmake_script_test(
  runtime.s101_quilt_smoke
  COMMAND "$<TARGET_FILE:s101_quilt_smoke_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}"
  QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
  QT_QPA_PLATFORM "offscreen")

add_executable(s101_symbolized_smoke_tests
  runtime/s101_symbolized_smoke_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/area_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/feature_layer_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/line_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/point_symbol_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projection_context.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projected_viewport.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/text_label_renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/display_priority_model.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/feature_symbolizer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/portrayal_registry.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s52_presentation_assets.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s101_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/portrayal/s57_rule_table.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/rhi_render_backend.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp")
target_link_libraries(
  s101_symbolized_smoke_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          PROJ::proj
          Qt6::GuiPrivate
          Catch2::Catch2WithMain)
target_include_directories(s101_symbolized_smoke_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(s101_symbolized_smoke_tests)
chart_view_copy_runtime_dlls(s101_symbolized_smoke_tests)

chart_view_add_cmake_script_test(
  runtime.s101_symbolized_smoke
  COMMAND "$<TARGET_FILE:s101_symbolized_smoke_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}"
  QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
  QT_QPA_PLATFORM "offscreen")

# ── Scene builder from SENC ───────────────────────────────────────
add_executable(scene_builder_tests
  runtime/scene_builder_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projection_context.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projected_viewport.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp")
target_link_libraries(
  scene_builder_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          PROJ::proj
          Catch2::Catch2WithMain)
target_include_directories(scene_builder_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(scene_builder_tests)
chart_view_copy_runtime_dlls(scene_builder_tests)

chart_view_add_cmake_script_test(
  runtime.scene_builder
  COMMAND "$<TARGET_FILE:scene_builder_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(chart_catalog_tests
  runtime/chart_catalog_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/catalog/chart_catalog.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp")
target_link_libraries(
  chart_catalog_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(chart_catalog_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(chart_catalog_tests)
chart_view_copy_runtime_dlls(chart_catalog_tests)

chart_view_add_cmake_script_test(
  runtime.chart_catalog
  COMMAND "$<TARGET_FILE:chart_catalog_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(coverage_index_tests
  runtime/coverage_index_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/catalog/chart_catalog.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/catalog/coverage_index.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projection_context.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projected_viewport.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp")
target_link_libraries(
  coverage_index_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          PROJ::proj
          Catch2::Catch2WithMain)
target_include_directories(coverage_index_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(coverage_index_tests)
chart_view_copy_runtime_dlls(coverage_index_tests)

chart_view_add_cmake_script_test(
  runtime.coverage_index
  COMMAND "$<TARGET_FILE:coverage_index_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(chart_selection_policy_tests
  runtime/chart_selection_policy_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/catalog/chart_selection_policy.cpp")
target_link_libraries(
  chart_selection_policy_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(chart_selection_policy_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(chart_selection_policy_tests)
chart_view_copy_runtime_dlls(chart_selection_policy_tests)

chart_view_add_cmake_script_test(
  runtime.chart_selection_policy
  COMMAND "$<TARGET_FILE:chart_selection_policy_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(quilt_plan_tests runtime/quilt_plan_tests.cpp)
target_link_libraries(
  quilt_plan_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(quilt_plan_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(quilt_plan_tests)
chart_view_copy_runtime_dlls(quilt_plan_tests)

chart_view_add_cmake_script_test(
  runtime.quilt_plan
  COMMAND "$<TARGET_FILE:quilt_plan_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(quilt_planner_tests
  runtime/quilt_planner_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/catalog/chart_catalog.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/catalog/coverage_index.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/catalog/chart_selection_policy.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projection_context.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/projection/projected_viewport.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/quilt/quilt_planner.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_reader.cpp"
  "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp")
target_link_libraries(
  quilt_planner_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          PROJ::proj
          Catch2::Catch2WithMain)
target_include_directories(quilt_planner_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(quilt_planner_tests)
chart_view_copy_runtime_dlls(quilt_planner_tests)

chart_view_add_cmake_script_test(
  runtime.quilt_planner
  COMMAND "$<TARGET_FILE:quilt_planner_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

add_executable(zoom_policy_tests
  runtime/zoom_policy_tests.cpp
  "${PROJECT_SOURCE_DIR}/src/runtime/quilt/zoom_policy.cpp")
target_link_libraries(
  zoom_policy_tests
  PRIVATE chart_view::chart_view_warnings
          chart_view::chart_view_options
          chart_view::chart_runtime
          Catch2::Catch2WithMain)
target_include_directories(zoom_policy_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

chart_view_configure_linker(zoom_policy_tests)
chart_view_copy_runtime_dlls(zoom_policy_tests)

chart_view_add_cmake_script_test(
  runtime.zoom_policy
  COMMAND "$<TARGET_FILE:zoom_policy_tests>"
  PATH_DEBUG "${chart_view_qt_test_path_debug}"
  PATH_RELEASE "${chart_view_qt_test_path_release}")

if(TARGET chart_qtwidgets)
  add_executable(qtwidgets_smoke_tests
    qtwidgets/qtwidgets_smoke_tests.cpp
    "${PROJECT_SOURCE_DIR}/src/runtime/senc/senc_writer.cpp")
  target_link_libraries(
    qtwidgets_smoke_tests
    PRIVATE chart_view::chart_view_warnings
            chart_view::chart_view_options
            chart_view::chart_qtwidgets
            Catch2::Catch2WithMain)
  target_include_directories(qtwidgets_smoke_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/runtime")

  chart_view_configure_linker(qtwidgets_smoke_tests)
  chart_view_copy_runtime_dlls(qtwidgets_smoke_tests)

  chart_view_add_cmake_script_test(
    qtwidgets.smoke
    COMMAND "$<TARGET_FILE:qtwidgets_smoke_tests>"
    PATH_DEBUG "${chart_view_qt_test_path_debug}"
    PATH_RELEASE "${chart_view_qt_test_path_release}"
    QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
    QT_QPA_PLATFORM "offscreen")
endif()

if(TARGET chart_standalone)
  chart_view_add_cmake_script_test(
    chart_standalone.version
    COMMAND "$<TARGET_FILE:chart_standalone>"
    ARGS --version
    PATH_DEBUG "${chart_view_qt_test_path_debug}"
    PATH_RELEASE "${chart_view_qt_test_path_release}")

  chart_view_add_cmake_script_test(
    chart_standalone.smoke
    COMMAND "$<TARGET_FILE:chart_standalone>"
    ARGS --smoke-test
    PATH_DEBUG "${chart_view_qt_test_path_debug}"
    PATH_RELEASE "${chart_view_qt_test_path_release}"
    QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
    QT_QPA_PLATFORM "offscreen")

  chart_view_add_cmake_script_test(
    chart_standalone.open_chart.smoke
    COMMAND "$<TARGET_FILE:chart_standalone>"
    ARGS --smoke-test --open-chart "${PROJECT_SOURCE_DIR}/tests/data/s101/smoke_dataset.101" --chart-type s101
    PATH_DEBUG "${chart_view_qt_test_path_debug}"
    PATH_RELEASE "${chart_view_qt_test_path_release}"
    QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
    QT_QPA_PLATFORM "offscreen")

  chart_view_add_cmake_script_test(
    chart_standalone.open_chart_directory.smoke
    COMMAND "$<TARGET_FILE:chart_standalone>"
    ARGS --smoke-test --open-chart-directory "${PROJECT_SOURCE_DIR}/tests/data/s101/quilt_directory"
    PATH_DEBUG "${chart_view_qt_test_path_debug}"
    PATH_RELEASE "${chart_view_qt_test_path_release}"
    QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
    QT_QPA_PLATFORM "offscreen")

  chart_view_add_cmake_script_test(
    chart_standalone.s101_host_smoke
    COMMAND "$<TARGET_FILE:chart_standalone>"
    ARGS --smoke-test --open-chart "${PROJECT_SOURCE_DIR}/tests/data/s101/smoke_dataset.101" --chart-type s101
    PATH_DEBUG "${chart_view_qt_test_path_debug}"
    PATH_RELEASE "${chart_view_qt_test_path_release}"
    QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
    QT_QPA_PLATFORM "offscreen")

  if(CHARTSYS_ENABLE_REAL_CHART_TESTS AND NOT CHARTSYS_S57_TESTDATA_ROOT STREQUAL "")
    chart_view_add_cmake_script_test(
      chart_standalone.s57_host_smoke
      COMMAND "$<TARGET_FILE:chart_standalone>"
      ARGS --smoke-test --open-chart "${CHARTSYS_S57_TESTDATA_ROOT}/C1511781.000" --chart-type s57
      PATH_DEBUG "${chart_view_qt_test_path_debug}"
      PATH_RELEASE "${chart_view_qt_test_path_release}"
      QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
      QT_QPA_PLATFORM "offscreen")
  endif()

  if(CHARTSYS_ENABLE_REAL_CHART_TESTS AND NOT CHARTSYS_CM93_TESTDATA_ROOT STREQUAL "")
    chart_view_add_cmake_script_test(
      chart_standalone.cm93_host_smoke
      COMMAND "$<TARGET_FILE:chart_standalone>"
      ARGS --smoke-test --open-chart "${CHARTSYS_CM93_TESTDATA_ROOT}" --chart-type cm93
      PATH_DEBUG "${chart_view_qt_test_path_debug}"
      PATH_RELEASE "${chart_view_qt_test_path_release}"
      QT_PLUGIN_PATH "${chart_view_QT_PLUGIN_DIR}"
      QT_QPA_PLATFORM "offscreen")
  endif()
endif()

if(TARGET chart_qtwidgets)
  set(chart_view_package_prefix_path "${CMAKE_BINARY_DIR}/package-smoke/install;${chart_view_QT_PREFIX_DIR}")
else()
  set(chart_view_package_prefix_path "${CMAKE_BINARY_DIR}/package-smoke/install")
endif()

configure_file(
  package_smoke/run_package_smoke.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/package_smoke/run_package_smoke.cmake"
  @ONLY)

add_test(
  NAME package.smoke
  COMMAND ${CMAKE_COMMAND} -DCHART_VIEW_TEST_CONFIG=$<CONFIG> -P "${CMAKE_CURRENT_BINARY_DIR}/package_smoke/run_package_smoke.cmake")
