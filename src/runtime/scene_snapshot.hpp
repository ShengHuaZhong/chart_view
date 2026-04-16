#ifndef CHART_VIEW_RUNTIME_SCENE_SNAPSHOT_HPP
#define CHART_VIEW_RUNTIME_SCENE_SNAPSHOT_HPP

#include "scene_model.hpp"
#include "viewport_state.hpp"

#include <chart_view/runtime/chart_runtime_types.h>

#include <cstdint>
#include <vector>

namespace chart_view::runtime {

// Immutable snapshot of a scene at a point in time.
// Produced from SceneModel + ViewportState, consumed by RenderScheduler.
class SceneSnapshot
{
public:
  SceneSnapshot(const SceneModel &model, const ViewportState &viewport)
    : m_charts(model.charts())
    , m_layers(model.layers())
    , m_viewport(viewport.viewport())
    , m_viewportRevision(viewport.revision())
  {
  }

  [[nodiscard]] const std::vector<SceneChartEntry> &charts() const noexcept { return m_charts; }
  [[nodiscard]] const std::vector<SceneLayerEntry> &layers() const noexcept { return m_layers; }
  [[nodiscard]] const chart_view_viewport_t &viewport() const noexcept { return m_viewport; }
  [[nodiscard]] std::uint64_t viewportRevision() const noexcept { return m_viewportRevision; }
  [[nodiscard]] bool empty() const noexcept { return m_layers.empty(); }

private:
  std::vector<SceneChartEntry> m_charts;
  std::vector<SceneLayerEntry> m_layers;
  chart_view_viewport_t m_viewport;
  std::uint64_t m_viewportRevision;
};

}// namespace chart_view::runtime

#endif
