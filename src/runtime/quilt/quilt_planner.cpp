#include "quilt_planner.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>

namespace chart_view::runtime::quilt {

QuiltPlan QuiltPlanner::build(
  const catalog::CoverageIndex &coverageIndex,
  const catalog::ChartSelectionPolicy &selectionPolicy,
  chart_data::Extent viewportExtent,
  double viewportScaleDenominator) const
{
  QuiltPlan plan;
  plan.setViewport(viewportExtent, viewportScaleDenominator);

  if(!viewportExtent.isValid() || viewportScaleDenominator <= 0.0) {
    return plan;
  }

  const auto candidates = coverageIndex.query(viewportExtent);
  const auto ranked = selectionPolicy.rankCandidates(candidates, viewportScaleDenominator);

  QuiltSelectionResult selection;
  selection.viewportScaleDenominator = viewportScaleDenominator;
  selection.candidateCount = candidates.size();
  selection.orderedChartIds.reserve(ranked.size());

  std::uint32_t drawOrder = 0;
  for(const auto *entry : ranked) {
    if(entry == nullptr) {
      continue;
    }

    const auto visibleExtent = intersectExtents(entry->extent, viewportExtent);
    if(!visibleExtent.isValid()) {
      continue;
    }

    selection.orderedChartIds.push_back(entry->id);
    plan.addLayer(QuiltLayer::fromCatalogEntry(*entry, drawOrder, visibleExtent));
    ++drawOrder;
  }

  plan.setSelectionResult(std::move(selection));
  return plan;
}

chart_data::Extent QuiltPlanner::intersectExtents(
  const chart_data::Extent &lhs,
  const chart_data::Extent &rhs) noexcept
{
  return {
    std::max(lhs.minLon, rhs.minLon),
    std::max(lhs.minLat, rhs.minLat),
    std::min(lhs.maxLon, rhs.maxLon),
    std::min(lhs.maxLat, rhs.maxLat)};
}

}// namespace chart_view::runtime::quilt
