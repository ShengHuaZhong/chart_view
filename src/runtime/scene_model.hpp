#ifndef CHART_VIEW_RUNTIME_SCENE_MODEL_HPP
#define CHART_VIEW_RUNTIME_SCENE_MODEL_HPP

#include <cstdint>
#include <vector>

namespace chart_view::runtime {

// Placeholder for a scene layer entry.
// Will grow when feature model and SENC reader are implemented.
struct SceneLayerEntry
{
  std::uint32_t layerId{0};
  std::uint32_t featureIndex{0};   // index into FeatureChartDataset::features()
  std::uint8_t  geometryType{0};   // chart_data::GeometryType ordinal
};

// Mutable scene model -- accumulates layers / features for the current frame.
class SceneModel
{
public:
  SceneModel() = default;

  void clear() noexcept { m_layers.clear(); }
  void addLayer(const SceneLayerEntry &entry) { m_layers.push_back(entry); }

  [[nodiscard]] const std::vector<SceneLayerEntry> &layers() const noexcept { return m_layers; }
  [[nodiscard]] bool empty() const noexcept { return m_layers.empty(); }
  [[nodiscard]] std::size_t layerCount() const noexcept { return m_layers.size(); }

private:
  std::vector<SceneLayerEntry> m_layers;
};

}// namespace chart_view::runtime

#endif
