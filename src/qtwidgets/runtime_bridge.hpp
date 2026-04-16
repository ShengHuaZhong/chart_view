#ifndef CHART_VIEW_QTWIDGETS_RUNTIME_BRIDGE_HPP
#define CHART_VIEW_QTWIDGETS_RUNTIME_BRIDGE_HPP

#include <chart_view/runtime/chart_runtime.h>

#include <cstdint>

namespace chart_view::qtwidgets {

// RuntimeBridge manages the connection between a QWidget host and a
// chart_view_runtime_t handle.  It tracks whether the runtime was
// created internally (and therefore should be destroyed by us) or
// was provided externally (and must outlive the bridge).
class RuntimeBridge
{
public:
  RuntimeBridge();
  ~RuntimeBridge();

  RuntimeBridge(const RuntimeBridge &) = delete;
  RuntimeBridge &operator=(const RuntimeBridge &) = delete;
  RuntimeBridge(RuntimeBridge &&) = delete;
  RuntimeBridge &operator=(RuntimeBridge &&) = delete;

  // Create and own an internal runtime handle.
  // Returns the status code from chart_view_runtime_create.
  chart_view_status_t createOwnedRuntime();

  // Attach an externally-owned runtime.  The caller keeps ownership.
  // Returns false if runtime is null or a runtime is already attached.
  bool attachExternal(chart_view_runtime_t *runtime);

  // Detach / release the current runtime.
  // If the bridge owns the runtime, it is destroyed.
  void detach();

  [[nodiscard]] bool hasRuntime() const noexcept { return m_runtime != nullptr; }
  [[nodiscard]] bool ownsRuntime() const noexcept { return m_owned; }
  [[nodiscard]] chart_view_runtime_t *handle() const noexcept { return m_runtime; }

  // Query runtime info.  Returns status_ok on success.
  [[nodiscard]] chart_view_status_t queryInfo(chart_view_runtime_info_t *out) const;
  [[nodiscard]] chart_view_status_t queryViewport(chart_view_viewport_t *out) const;
  [[nodiscard]] chart_view_status_t queryFrameBufferInfo(chart_view_frame_buffer_info_t *out) const;
  [[nodiscard]] chart_view_status_t stepZoom(std::int32_t stepCount, chart_view_zoom_result_t *out) const;
  [[nodiscard]] chart_view_status_t queryS52MarinerSettings(chart_view_s52_mariner_settings_t *out) const;
  [[nodiscard]] chart_view_status_t queryS57ClassFilters(chart_view_s57_class_filter_t *out, std::uint32_t *inoutCount) const;
  [[nodiscard]] chart_view_status_t queryS52RuleFilters(chart_view_s52_rule_filter_t *out, std::uint32_t *inoutCount) const;
  [[nodiscard]] chart_view_status_t enumerateS52Rules(chart_view_s52_rule_descriptor_t *out, std::uint32_t *inoutCount) const;

  // Set viewport on the runtime.
  [[nodiscard]] chart_view_status_t setViewport(const chart_view_viewport_t &vp) const;
  [[nodiscard]] chart_view_status_t setS52MarinerSettings(const chart_view_s52_mariner_settings_t &settings) const;
  [[nodiscard]] chart_view_status_t setS57ClassFilters(const chart_view_s57_class_filter_t *filters,
                                                       std::uint32_t filterCount) const;
  [[nodiscard]] chart_view_status_t setS52RuleFilters(const chart_view_s52_rule_filter_t *filters,
                                                      std::uint32_t filterCount) const;

  // Load SENC blob into the runtime.
  [[nodiscard]] chart_view_status_t loadSenc(const void *data, std::uint32_t size) const;

  // Render one frame and return the result.
  [[nodiscard]] chart_view_status_t renderFrame(chart_view_render_frame_result_t *out) const;
  [[nodiscard]] chart_view_status_t copyFrameRgba(void *dst, std::uint32_t size) const;

private:
  chart_view_runtime_t *m_runtime = nullptr;
  bool m_owned = false;
};

}// namespace chart_view::qtwidgets

#endif
