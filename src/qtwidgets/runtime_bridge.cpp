#include "runtime_bridge.hpp"

namespace chart_view::qtwidgets {

RuntimeBridge::RuntimeBridge() = default;

RuntimeBridge::~RuntimeBridge() { detach(); }

chart_view_status_t RuntimeBridge::createOwnedRuntime()
{
  if(m_runtime != nullptr) {
    return chart_view_status_already_initialized;
  }

  const auto status = chart_view_runtime_create(&m_runtime);
  if(status == chart_view_status_ok) {
    m_owned = true;
  }

  return status;
}

bool RuntimeBridge::attachExternal(chart_view_runtime_t *runtime)
{
  if(runtime == nullptr || m_runtime != nullptr) {
    return false;
  }

  m_runtime = runtime;
  m_owned = false;
  return true;
}

void RuntimeBridge::detach()
{
  if(m_runtime != nullptr && m_owned) {
    chart_view_runtime_destroy(m_runtime);
  }

  m_runtime = nullptr;
  m_owned = false;
}

chart_view_status_t RuntimeBridge::queryInfo(chart_view_runtime_info_t *out) const
{
  if(m_runtime == nullptr) {
    return chart_view_status_not_initialized;
  }

  return chart_view_runtime_get_info(m_runtime, out);
}

chart_view_status_t RuntimeBridge::queryViewport(chart_view_viewport_t *out) const
{
  if(m_runtime == nullptr) {
    return chart_view_status_not_initialized;
  }

  return chart_view_runtime_get_viewport(m_runtime, out);
}

chart_view_status_t RuntimeBridge::queryFrameBufferInfo(chart_view_frame_buffer_info_t *out) const
{
  if(m_runtime == nullptr) {
    return chart_view_status_not_initialized;
  }

  return chart_view_runtime_get_frame_buffer_info(m_runtime, out);
}

chart_view_status_t RuntimeBridge::setViewport(const chart_view_viewport_t &vp) const
{
  if(m_runtime == nullptr) {
    return chart_view_status_not_initialized;
  }

  return chart_view_runtime_set_viewport(m_runtime, &vp);
}

chart_view_status_t RuntimeBridge::loadSenc(const void *data, std::uint32_t size) const
{
  if(m_runtime == nullptr) {
    return chart_view_status_not_initialized;
  }

  return chart_view_runtime_load_senc(m_runtime, data, size);
}

chart_view_status_t RuntimeBridge::renderFrame(chart_view_render_frame_result_t *out) const
{
  if(m_runtime == nullptr) {
    return chart_view_status_not_initialized;
  }

  return chart_view_runtime_render_frame(m_runtime, out);
}

chart_view_status_t RuntimeBridge::copyFrameRgba(void *dst, std::uint32_t size) const
{
  if(m_runtime == nullptr) {
    return chart_view_status_not_initialized;
  }

  return chart_view_runtime_copy_frame_rgba(m_runtime, dst, size);
}

}// namespace chart_view::qtwidgets
