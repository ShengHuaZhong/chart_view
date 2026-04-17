#include <chart_view/runtime/chart_runtime.h>

#include "runtime_context.hpp"

#include <internal_use_only/config.hpp>

#include <QByteArray>
#include <QGuiApplication>

#include <cmath>
#include <new>
#include <span>

struct chart_view_runtime
{
  chart_view::runtime::RuntimeContext context;
};

namespace {

using chart_view::runtime::portrayal::S52ColorScheme;
using chart_view::runtime::portrayal::S52DisplayCategory;
using chart_view::runtime::portrayal::S52DisplaySettings;
using chart_view::runtime::portrayal::S52PointSymbolMode;

QGuiApplication *ensureRuntimeGuiApplication()
{
  if(QGuiApplication::instance() != nullptr) {
    return static_cast<QGuiApplication *>(QGuiApplication::instance());
  }

  static QGuiApplication *headlessApp = [] {
    if(qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
      qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    }

    static int argc = 1;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
    static char arg0[] = "chart_runtime";
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
    static char *argv[] = {arg0, nullptr};
    return new QGuiApplication(argc, argv);
  }();

  return headlessApp;
}

bool isFiniteNonNegative(double value) noexcept
{
  return std::isfinite(value) && value >= 0.0;
}

bool convertMarinerSettings(
  const chart_view_s52_mariner_settings_t &dto,
  S52DisplaySettings &out) noexcept
{
  switch(dto.palette) {
  case chart_view_s52_palette_day:
    out.colorScheme = S52ColorScheme::kDay;
    break;
  case chart_view_s52_palette_dusk:
    out.colorScheme = S52ColorScheme::kDusk;
    break;
  case chart_view_s52_palette_night:
    out.colorScheme = S52ColorScheme::kNight;
    break;
  default:
    return false;
  }

  switch(dto.display_category) {
  case chart_view_s52_display_base:
    out.displayCategory = S52DisplayCategory::kDisplayBase;
    break;
  case chart_view_s52_display_standard:
    out.displayCategory = S52DisplayCategory::kStandard;
    break;
  case chart_view_s52_display_all:
    out.displayCategory = S52DisplayCategory::kAll;
    break;
  default:
    return false;
  }

  if(!isFiniteNonNegative(dto.safety_contour_m) || !isFiniteNonNegative(dto.safety_depth_m) ||
     !isFiniteNonNegative(dto.shallow_contour_m) || !isFiniteNonNegative(dto.deep_contour_m)) {
    return false;
  }

  out.pointSymbolMode =
    dto.simplified_points != 0U ? S52PointSymbolMode::kSimplified : S52PointSymbolMode::kTraditional;
  out.showSoundings = dto.show_soundings != 0U;
  out.showTextLabels = dto.show_text != 0U;
  out.twoShades = dto.two_shades != 0U;
  out.safetyContourMeters = dto.safety_contour_m;
  out.safetyDepthMeters = dto.safety_depth_m;
  out.shallowContourMeters = dto.shallow_contour_m;
  out.deepContourMeters = dto.deep_contour_m;
  out.shallowPattern = dto.shallow_pattern != 0U;
  out.fullSectorLights = dto.full_sector_lights != 0U;
  out.symbolizedBoundaries = dto.symbolized_boundaries != 0U;
  out.honorScamin = dto.honor_scamin != 0U;
  return true;
}

}// namespace

extern "C" {

std::uint32_t chart_view_runtime_abi_version(void) { return CHART_VIEW_RUNTIME_ABI_VERSION; }

const char *chart_view_runtime_version_string(void) { return chart_view::build::project_version.data(); }

chart_view_status_t chart_view_runtime_create(chart_view_runtime_t **out_runtime)
{
  if(out_runtime == nullptr) {
    return chart_view_status_invalid_argument;
  }

  try {
    (void)ensureRuntimeGuiApplication();
    *out_runtime = new chart_view_runtime{};
    return chart_view_status_ok;
  } catch(const std::bad_alloc &) {
    *out_runtime = nullptr;
    return chart_view_status_allocation_failure;
  }
}

void chart_view_runtime_destroy(chart_view_runtime_t *runtime)
{
  delete runtime;
}

chart_view_status_t chart_view_runtime_initialize(chart_view_runtime_t *runtime)
{
  if(runtime == nullptr) {
    return chart_view_status_invalid_argument;
  }

  return runtime->context.initialize();
}

chart_view_status_t chart_view_runtime_shutdown(chart_view_runtime_t *runtime)
{
  if(runtime == nullptr) {
    return chart_view_status_invalid_argument;
  }

  return runtime->context.shutdown();
}

chart_view_status_t chart_view_runtime_get_info(
  const chart_view_runtime_t *runtime,
  chart_view_runtime_info_t *out_info)
{
  if(runtime == nullptr || out_info == nullptr) {
    return chart_view_status_invalid_argument;
  }

  *out_info = runtime->context.info();
  return chart_view_status_ok;
}

chart_view_status_t chart_view_runtime_get_loaded_chart_info(
  const chart_view_runtime_t *runtime,
  chart_view_loaded_chart_info_t *out_info)
{
  if(runtime == nullptr || out_info == nullptr) {
    return chart_view_status_invalid_argument;
  }

  runtime->context.getLoadedChartInfo(*out_info);
  return chart_view_status_ok;
}

chart_view_status_t chart_view_runtime_get_viewport(
  const chart_view_runtime_t *runtime,
  chart_view_viewport_t *out_viewport)
{
  if(runtime == nullptr || out_viewport == nullptr) {
    return chart_view_status_invalid_argument;
  }

  runtime->context.getViewport(*out_viewport);
  return chart_view_status_ok;
}

chart_view_status_t chart_view_runtime_set_viewport(
  chart_view_runtime_t *runtime,
  const chart_view_viewport_t *viewport)
{
  if(runtime == nullptr || viewport == nullptr) {
    return chart_view_status_invalid_argument;
  }

  return runtime->context.setViewport(*viewport);
}

chart_view_status_t chart_view_runtime_step_zoom(
  chart_view_runtime_t *runtime,
  std::int32_t step_count,
  chart_view_zoom_result_t *out_result)
{
  if(runtime == nullptr || out_result == nullptr) {
    return chart_view_status_invalid_argument;
  }

  return runtime->context.stepZoom(step_count, *out_result);
}

chart_view_status_t chart_view_runtime_load_senc(
  chart_view_runtime_t *runtime,
  const void *data,
  std::uint32_t size)
{
  if(runtime == nullptr || data == nullptr || size == 0) {
    return chart_view_status_invalid_argument;
  }

  auto span = std::span<const std::uint8_t>(
    static_cast<const std::uint8_t *>(data), size);
  return runtime->context.loadSenc(span);
}

chart_view_status_t chart_view_runtime_open_chart_file(
  chart_view_runtime_t *runtime,
  const char *path,
  chart_view_chart_source_type_t source_type)
{
  if(runtime == nullptr || path == nullptr || path[0] == '\0') {
    return chart_view_status_invalid_argument;
  }

  return runtime->context.openChartFile(path, source_type);
}

chart_view_status_t chart_view_runtime_open_chart_directory(
  chart_view_runtime_t *runtime,
  const char *path)
{
  if(runtime == nullptr || path == nullptr || path[0] == '\0') {
    return chart_view_status_invalid_argument;
  }

  return runtime->context.openChartDirectory(path);
}

chart_view_status_t chart_view_runtime_set_s52_mariner_settings(
  chart_view_runtime_t *runtime,
  const chart_view_s52_mariner_settings_t *settings)
{
  if(runtime == nullptr || settings == nullptr) {
    return chart_view_status_invalid_argument;
  }

  S52DisplaySettings converted{};
  if(!convertMarinerSettings(*settings, converted)) {
    return chart_view_status_invalid_argument;
  }

  return runtime->context.setS52MarinerSettings(converted);
}

chart_view_status_t chart_view_runtime_get_s52_mariner_settings(
  const chart_view_runtime_t *runtime,
  chart_view_s52_mariner_settings_t *out_settings)
{
  if(runtime == nullptr || out_settings == nullptr) {
    return chart_view_status_invalid_argument;
  }

  runtime->context.getS52MarinerSettings(*out_settings);
  return chart_view_status_ok;
}

chart_view_status_t chart_view_runtime_set_s57_class_filters(
  chart_view_runtime_t *runtime,
  const chart_view_s57_class_filter_t *filters,
  std::uint32_t filter_count)
{
  if(runtime == nullptr) {
    return chart_view_status_invalid_argument;
  }

  if(filter_count > 0U && filters == nullptr) {
    return chart_view_status_invalid_argument;
  }

  return runtime->context.setS57ClassFilters(
    std::span<const chart_view_s57_class_filter_t>(filters, filter_count));
}

chart_view_status_t chart_view_runtime_get_s57_class_filters(
  const chart_view_runtime_t *runtime,
  chart_view_s57_class_filter_t *out_filters,
  std::uint32_t *inout_filter_count)
{
  if(runtime == nullptr || inout_filter_count == nullptr) {
    return chart_view_status_invalid_argument;
  }

  return runtime->context.getS57ClassFilters(out_filters, *inout_filter_count);
}

chart_view_status_t chart_view_runtime_set_s52_rule_filters(
  chart_view_runtime_t *runtime,
  const chart_view_s52_rule_filter_t *filters,
  std::uint32_t filter_count)
{
  if(runtime == nullptr) {
    return chart_view_status_invalid_argument;
  }

  if(filter_count > 0U && filters == nullptr) {
    return chart_view_status_invalid_argument;
  }

  return runtime->context.setS52RuleFilters(
    std::span<const chart_view_s52_rule_filter_t>(filters, filter_count));
}

chart_view_status_t chart_view_runtime_get_s52_rule_filters(
  const chart_view_runtime_t *runtime,
  chart_view_s52_rule_filter_t *out_filters,
  std::uint32_t *inout_filter_count)
{
  if(runtime == nullptr || inout_filter_count == nullptr) {
    return chart_view_status_invalid_argument;
  }

  return runtime->context.getS52RuleFilters(out_filters, *inout_filter_count);
}

chart_view_status_t chart_view_runtime_enumerate_s52_rules(
  const chart_view_runtime_t *runtime,
  chart_view_s52_rule_descriptor_t *out_rules,
  std::uint32_t *inout_rule_count)
{
  if(runtime == nullptr || inout_rule_count == nullptr) {
    return chart_view_status_invalid_argument;
  }

  return runtime->context.enumerateS52Rules(out_rules, *inout_rule_count);
}

chart_view_status_t chart_view_runtime_query_features_at_point(
  const chart_view_runtime_t *runtime,
  const chart_view_feature_query_t *query,
  chart_view_feature_summary_t *out_features,
  std::uint32_t *inout_feature_count)
{
  if(runtime == nullptr || query == nullptr || inout_feature_count == nullptr) {
    return chart_view_status_invalid_argument;
  }

  return runtime->context.queryFeaturesAtPoint(*query, out_features, *inout_feature_count);
}

chart_view_status_t chart_view_runtime_describe_feature(
  const chart_view_runtime_t *runtime,
  std::uint32_t runtime_feature_token,
  chart_view_feature_summary_t *out_summary)
{
  if(runtime == nullptr || out_summary == nullptr || runtime_feature_token == 0U) {
    return chart_view_status_invalid_argument;
  }

  return runtime->context.describeFeature(runtime_feature_token, *out_summary);
}

chart_view_status_t chart_view_runtime_render_frame(
  chart_view_runtime_t *runtime,
  chart_view_render_frame_result_t *out_result)
{
  if(runtime == nullptr || out_result == nullptr) {
    return chart_view_status_invalid_argument;
  }

  return runtime->context.renderFrame(*out_result);
}

chart_view_status_t chart_view_runtime_get_frame_buffer_info(
  const chart_view_runtime_t *runtime,
  chart_view_frame_buffer_info_t *out_info)
{
  if(runtime == nullptr || out_info == nullptr) {
    return chart_view_status_invalid_argument;
  }

  runtime->context.getFrameBufferInfo(*out_info);
  return chart_view_status_ok;
}

chart_view_status_t chart_view_runtime_copy_frame_rgba(
  const chart_view_runtime_t *runtime,
  void *dst_rgba,
  std::uint32_t dst_size_bytes)
{
  if(runtime == nullptr || dst_rgba == nullptr || dst_size_bytes == 0) {
    return chart_view_status_invalid_argument;
  }

  auto span = std::span<std::uint8_t>(static_cast<std::uint8_t *>(dst_rgba), dst_size_bytes);
  return runtime->context.copyFrameRgba(span);
}
}
