#ifndef CHART_VIEW_RUNTIME_QUILT_ZOOM_POLICY_HPP
#define CHART_VIEW_RUNTIME_QUILT_ZOOM_POLICY_HPP

#include "quilt_plan.hpp"

#include <cstddef>
#include <cstdint>

namespace chart_view::runtime::quilt {

enum class ZoomScaleState : std::uint8_t
{
  kNormal = 0,
  kOverzoom = 1,
  kUnderzoom = 2,
  kNoCharts = 3
};

enum class ZoomRebuildReason : std::uint8_t
{
  kNone = 0,
  kEmptyPlan = 1,
  kPreferredChartChanged = 2,
  kOverzoom = 3,
  kUnderzoom = 4
};

struct ZoomDecision
{
  double currentScaleDenominator{0.0};
  double requestedScaleDenominator{0.0};
  double resolvedScaleDenominator{0.0};
  std::size_t preferredLayerIndex{0};
  ZoomScaleState primaryScaleState{ZoomScaleState::kNoCharts};
  bool shouldRebuildPlan{false};
  ZoomRebuildReason rebuildReason{ZoomRebuildReason::kNone};
};

class ZoomPolicy
{
public:
  ZoomPolicy() = default;

  [[nodiscard]] double zoomIn(
    double currentScaleDenominator,
    std::uint32_t steps = 1) const noexcept;

  [[nodiscard]] double zoomOut(
    double currentScaleDenominator,
    std::uint32_t steps = 1) const noexcept;

  [[nodiscard]] ZoomDecision evaluate(
    const QuiltPlan &plan,
    double requestedScaleDenominator) const noexcept;

private:
  [[nodiscard]] static double clampScale(double scaleDenominator) noexcept;
  [[nodiscard]] static double scaleDistance(
    const QuiltLayer &layer,
    double scaleDenominator) noexcept;
  [[nodiscard]] static std::size_t preferredLayerIndex(
    const QuiltPlan &plan,
    double scaleDenominator) noexcept;
  [[nodiscard]] static ZoomScaleState classifyPrimaryScale(
    const QuiltPlan &plan,
    double scaleDenominator) noexcept;
};

}// namespace chart_view::runtime::quilt

#endif
