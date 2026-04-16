#include "zoom_policy.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace chart_view::runtime::quilt {

namespace {

constexpr double kStepFactor = 1.25;
constexpr double kMinScaleDenominator = 1000.0;
constexpr double kMaxScaleDenominator = 50000000.0;
constexpr double kOverzoomFactor = 2.0;
constexpr double kUnderzoomFactor = 2.0;

}// namespace

double ZoomPolicy::zoomIn(double currentScaleDenominator, std::uint32_t steps) const noexcept
{
  if(currentScaleDenominator <= 0.0) {
    return 0.0;
  }

  const auto stepped = currentScaleDenominator / std::pow(kStepFactor, static_cast<double>(steps));
  return clampScale(stepped);
}

double ZoomPolicy::zoomOut(double currentScaleDenominator, std::uint32_t steps) const noexcept
{
  if(currentScaleDenominator <= 0.0) {
    return 0.0;
  }

  const auto stepped = currentScaleDenominator * std::pow(kStepFactor, static_cast<double>(steps));
  return clampScale(stepped);
}

ZoomDecision ZoomPolicy::evaluate(
  const QuiltPlan &plan,
  double requestedScaleDenominator) const noexcept
{
  ZoomDecision decision;
  decision.currentScaleDenominator = plan.viewportScaleDenominator();
  decision.requestedScaleDenominator = requestedScaleDenominator;
  decision.resolvedScaleDenominator = clampScale(requestedScaleDenominator);

  if(plan.empty()) {
    decision.shouldRebuildPlan = true;
    decision.rebuildReason = ZoomRebuildReason::kEmptyPlan;
    return decision;
  }

  decision.preferredLayerIndex = preferredLayerIndex(plan, decision.resolvedScaleDenominator);
  decision.primaryScaleState = classifyPrimaryScale(plan, decision.resolvedScaleDenominator);

  if(decision.preferredLayerIndex != 0) {
    decision.shouldRebuildPlan = true;
    decision.rebuildReason = ZoomRebuildReason::kPreferredChartChanged;
    return decision;
  }

  if(decision.primaryScaleState == ZoomScaleState::kOverzoom) {
    decision.shouldRebuildPlan = true;
    decision.rebuildReason = ZoomRebuildReason::kOverzoom;
    return decision;
  }

  if(decision.primaryScaleState == ZoomScaleState::kUnderzoom) {
    decision.shouldRebuildPlan = true;
    decision.rebuildReason = ZoomRebuildReason::kUnderzoom;
    return decision;
  }

  return decision;
}

double ZoomPolicy::clampScale(double scaleDenominator) noexcept
{
  if(scaleDenominator <= 0.0) {
    return 0.0;
  }

  return std::clamp(scaleDenominator, kMinScaleDenominator, kMaxScaleDenominator);
}

double ZoomPolicy::scaleDistance(
  const QuiltLayer &layer,
  double scaleDenominator) noexcept
{
  if(layer.nativeScale <= 0.0 || scaleDenominator <= 0.0) {
    return std::numeric_limits<double>::max();
  }

  return std::abs(std::log(layer.nativeScale / scaleDenominator));
}

std::size_t ZoomPolicy::preferredLayerIndex(
  const QuiltPlan &plan,
  double scaleDenominator) noexcept
{
  const auto &layers = plan.layers();
  if(layers.empty() || scaleDenominator <= 0.0) {
    return 0;
  }

  std::size_t bestIndex = 0;
  auto bestDistance = scaleDistance(layers.front(), scaleDenominator);

  for(std::size_t i = 1; i < layers.size(); ++i) {
    const auto distance = scaleDistance(layers[i], scaleDenominator);
    if(distance < bestDistance) {
      bestIndex = i;
      bestDistance = distance;
    }
  }

  return bestIndex;
}

ZoomScaleState ZoomPolicy::classifyPrimaryScale(
  const QuiltPlan &plan,
  double scaleDenominator) noexcept
{
  if(plan.empty() || scaleDenominator <= 0.0) {
    return ZoomScaleState::kNoCharts;
  }

  const auto &primary = plan.layers().front();
  if(primary.nativeScale <= 0.0) {
    return ZoomScaleState::kNoCharts;
  }

  if(scaleDenominator < (primary.nativeScale / kOverzoomFactor)) {
    return ZoomScaleState::kOverzoom;
  }

  if(scaleDenominator > (primary.nativeScale * kUnderzoomFactor)) {
    return ZoomScaleState::kUnderzoom;
  }

  return ZoomScaleState::kNormal;
}

}// namespace chart_view::runtime::quilt
