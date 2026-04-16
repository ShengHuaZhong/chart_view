#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_DISPLAY_PRIORITY_MODEL_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_DISPLAY_PRIORITY_MODEL_HPP

#include "feature_symbolizer.hpp"

#include <cstdint>

namespace chart_view::runtime::chart_data {
struct Feature;
}

namespace chart_view::runtime::portrayal {

enum class DisplayLayerGroup : std::uint8_t
{
  kAreas = 0,
  kLines = 1,
  kPoints = 2,
  kText = 3,
};

struct DisplayPriority
{
  DisplayLayerGroup layerGroup{DisplayLayerGroup::kAreas};
  std::uint16_t priority{0};
};

class DisplayPriorityModel
{
public:
  DisplayPriorityModel() = default;

  [[nodiscard]] DisplayPriority resolve(
    const chart_data::Feature &feature,
    const FeatureSymbolization &symbolization) const noexcept;

  [[nodiscard]] static std::uint8_t layerGroupOrder(DisplayLayerGroup group) noexcept
  {
    return static_cast<std::uint8_t>(group);
  }
};

}// namespace chart_view::runtime::portrayal

#endif
