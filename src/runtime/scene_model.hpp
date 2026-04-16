#ifndef CHART_VIEW_RUNTIME_SCENE_MODEL_HPP
#define CHART_VIEW_RUNTIME_SCENE_MODEL_HPP

#include <chart_view/runtime/chart_runtime_types.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace chart_view::runtime {

struct SceneChartEntry
{
  std::string chartId;
  chart_view_chart_source_type_t sourceType{chart_view_chart_source_unknown};
  std::uint32_t drawOrder{0};
};

struct SceneLayerEntry
{
  std::uint32_t sourceChartIndex{0};  // index into SceneSnapshot::charts()
  std::uint32_t layerId{0};
  std::uint32_t featureIndex{0};   // index into FeatureChartDataset::features()
  std::uint8_t  geometryType{0};   // chart_data::GeometryType ordinal
};

// Mutable scene model -- accumulates layers / features for the current frame.
class SceneModel
{
public:
  SceneModel() = default;

  void clear() noexcept
  {
    m_charts.clear();
    m_layers.clear();
  }

  [[nodiscard]] std::uint32_t addChart(SceneChartEntry entry)
  {
    m_charts.push_back(std::move(entry));
    return static_cast<std::uint32_t>(m_charts.size() - 1);
  }

  void addLayer(const SceneLayerEntry &entry) { m_layers.push_back(entry); }

  [[nodiscard]] const std::vector<SceneChartEntry> &charts() const noexcept { return m_charts; }
  [[nodiscard]] const std::vector<SceneLayerEntry> &layers() const noexcept { return m_layers; }
  [[nodiscard]] bool empty() const noexcept { return m_layers.empty(); }
  [[nodiscard]] std::size_t chartCount() const noexcept { return m_charts.size(); }
  [[nodiscard]] std::size_t layerCount() const noexcept { return m_layers.size(); }

private:
  std::vector<SceneChartEntry> m_charts;
  std::vector<SceneLayerEntry> m_layers;
};

}// namespace chart_view::runtime

#endif
