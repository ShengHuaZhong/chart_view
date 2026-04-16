#ifndef CHART_VIEW_RUNTIME_RUNTIME_CONTEXT_HPP
#define CHART_VIEW_RUNTIME_RUNTIME_CONTEXT_HPP

#include "feature_layer_renderer.hpp"
#include "render_scheduler.hpp"
#include "rhi_render_backend.hpp"
#include "scene_model.hpp"
#include "viewport_state.hpp"
#include "catalog/chart_catalog.hpp"
#include "catalog/chart_selection_policy.hpp"
#include "catalog/coverage_index.hpp"
#include "chart_data/feature_chart_dataset.hpp"
#include "quilt/quilt_plan.hpp"

#include <chart_view/runtime/chart_runtime_types.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace chart_view::runtime {

enum class RuntimeState : std::uint8_t
{
  kCreated,
  kInitialized,
  kShutDown
};

class RuntimeContext
{
public:
  RuntimeContext();
  ~RuntimeContext();

  RuntimeContext(const RuntimeContext &) = delete;
  RuntimeContext &operator=(const RuntimeContext &) = delete;
  RuntimeContext(RuntimeContext &&) = delete;
  RuntimeContext &operator=(RuntimeContext &&) = delete;

  chart_view_status_t initialize();
  chart_view_status_t shutdown();

  [[nodiscard]] RuntimeState state() const noexcept { return m_state; }
  [[nodiscard]] bool isInitialized() const noexcept { return m_state == RuntimeState::kInitialized; }
  [[nodiscard]] const chart_view_runtime_info_t &info() const noexcept { return m_info; }

  [[nodiscard]] ViewportState &viewportState() noexcept { return m_viewport; }
  [[nodiscard]] const ViewportState &viewportState() const noexcept { return m_viewport; }

  [[nodiscard]] SceneModel &sceneModel() noexcept { return m_scene; }
  [[nodiscard]] const SceneModel &sceneModel() const noexcept { return m_scene; }

  [[nodiscard]] RenderScheduler &renderScheduler() noexcept { return m_scheduler; }
  [[nodiscard]] const RenderScheduler &renderScheduler() const noexcept { return m_scheduler; }

  // -- render frame API --
  chart_view_status_t setViewport(const chart_view_viewport_t &vp);
  chart_view_status_t stepZoom(std::int32_t stepCount, chart_view_zoom_result_t &out);
  chart_view_status_t loadSenc(std::span<const std::uint8_t> data);
  chart_view_status_t openChartFile(std::string_view path, chart_view_chart_source_type_t sourceType);
  chart_view_status_t openChartDirectory(std::string_view path);
  void getLoadedChartInfo(chart_view_loaded_chart_info_t &out) const;
  void getViewport(chart_view_viewport_t &out) const;
  chart_view_status_t renderFrame(chart_view_render_frame_result_t &result);
  void getFrameBufferInfo(chart_view_frame_buffer_info_t &out) const;
  chart_view_status_t copyFrameRgba(std::span<std::uint8_t> dst) const;

  [[nodiscard]] bool hasDataset() const noexcept { return m_dataset != nullptr; }

private:
  chart_view_status_t ensureRenderTarget();
  void clearLoadedCharts();
  void clearCatalogCache();
  chart_view_status_t rebuildDirectoryPlan();

  RuntimeState m_state = RuntimeState::kCreated;
  chart_view_runtime_info_t m_info{};
  ViewportState m_viewport;
  SceneModel m_scene;
  RenderScheduler m_scheduler;
  RhiRenderBackend m_renderBackend;
  FeatureLayerRenderer m_renderer;
  std::unique_ptr<chart_data::FeatureChartDataset> m_dataset;
  catalog::ChartCatalog m_catalog;
  catalog::CoverageIndex m_coverageIndex;
  catalog::ChartSelectionPolicy m_selectionPolicy;
  quilt::QuiltPlan m_quiltPlan;
  std::vector<chart_data::FeatureChartDataset> m_quiltDatasets;
  std::filesystem::path m_catalogCacheDirectory;
  bool m_directoryMode{false};
};

}// namespace chart_view::runtime

#endif
