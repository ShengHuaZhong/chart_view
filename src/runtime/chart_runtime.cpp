#include <chart_view/runtime/chart_runtime.h>

#include "runtime_context.hpp"

#include <internal_use_only/config.hpp>

#include <new>
#include <span>

struct chart_view_runtime
{
  chart_view::runtime::RuntimeContext context;
};

extern "C" {
std::uint32_t chart_view_runtime_abi_version(void) { return CHART_VIEW_RUNTIME_ABI_VERSION; }

const char *chart_view_runtime_version_string(void) { return chart_view::build::project_version.data(); }

chart_view_status_t chart_view_runtime_create(chart_view_runtime_t **out_runtime)
{
  if(out_runtime == nullptr) {
    return chart_view_status_invalid_argument;
  }

  try {
    *out_runtime = new chart_view_runtime{};
    return chart_view_status_ok;
  } catch(const std::bad_alloc &) {
    *out_runtime = nullptr;
    return chart_view_status_allocation_failure;
  }
}

void chart_view_runtime_destroy(chart_view_runtime_t *runtime) { delete runtime; }

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
