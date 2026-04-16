#include "quilt_planner.hpp"

#include "../projection/projected_bounds.hpp"

#include <algorithm>
#include <cstdint>
#include <span>
#include <utility>

namespace chart_view::runtime::quilt {

namespace {

constexpr projection::ProjectedExtent kInvalidProjectedExtent{1.0, 1.0, 0.0, 0.0};

[[nodiscard]] bool hasPositiveArea(const projection::ProjectedExtent &extent) noexcept
{
  return extent.isValid() && extent.minX < extent.maxX && extent.minY < extent.maxY;
}

[[nodiscard]] projection::ProjectedExtent intersectProjectedExtentsLocal(
  const projection::ProjectedExtent &lhs,
  const projection::ProjectedExtent &rhs) noexcept
{
  return {
    std::max(lhs.minX, rhs.minX),
    std::max(lhs.minY, rhs.minY),
    std::min(lhs.maxX, rhs.maxX),
    std::min(lhs.maxY, rhs.maxY)};
}

std::vector<projection::ProjectedExtent> subtractPatch(
  const projection::ProjectedExtent &source,
  const projection::ProjectedExtent &mask)
{
  if(!hasPositiveArea(source) || !projection::projectedExtentsOverlap(source, mask)) {
    return {source};
  }

  const auto overlap = intersectProjectedExtentsLocal(source, mask);
  if(!hasPositiveArea(overlap)) {
    return {source};
  }

  std::vector<projection::ProjectedExtent> result;
  result.reserve(4);

  const auto pushIfVisible = [&](projection::ProjectedExtent extent) {
    if(hasPositiveArea(extent)) {
      result.push_back(extent);
    }
  };

  pushIfVisible({source.minX, source.minY, overlap.minX, source.maxY});
  pushIfVisible({overlap.maxX, source.minY, source.maxX, source.maxY});
  pushIfVisible({overlap.minX, source.minY, overlap.maxX, overlap.minY});
  pushIfVisible({overlap.minX, overlap.maxY, overlap.maxX, source.maxY});

  return result;
}

std::vector<projection::ProjectedExtent> clipPatchesAgainstOwnedRegions(
  std::vector<projection::ProjectedExtent> patches,
  std::span<const projection::ProjectedExtent> ownedRegions)
{
  for(const auto &ownedRegion : ownedRegions) {
    std::vector<projection::ProjectedExtent> clipped;
    for(const auto &patch : patches) {
      const auto fragments = subtractPatch(patch, ownedRegion);
      clipped.insert(clipped.end(), fragments.begin(), fragments.end());
    }
    patches = std::move(clipped);
    if(patches.empty()) {
      break;
    }
  }

  return patches;
}

}// namespace

QuiltPlan QuiltPlanner::build(
  const catalog::CoverageIndex &coverageIndex,
  const catalog::ChartSelectionPolicy &selectionPolicy,
  chart_data::Extent viewportExtent,
  double viewportScaleDenominator) const
{
  QuiltPlan plan;
  plan.setViewport(viewportExtent, viewportScaleDenominator, kInvalidProjectedExtent);

  if(!viewportExtent.isValid() || viewportScaleDenominator <= 0.0) {
    return plan;
  }

  const auto projectionContext = projection::ProjectionContext::createMercator();
  if(!projectionContext.isValid()) {
    return plan;
  }

  projection::ProjectedExtent projectedViewportExtent{};
  if(!projectionContext.projectExtent(viewportExtent, projectedViewportExtent)
     || !hasPositiveArea(projectedViewportExtent)) {
    return plan;
  }

  plan.setViewport(viewportExtent, viewportScaleDenominator, projectedViewportExtent);

  const auto candidates = coverageIndex.query(viewportExtent);
  const auto ranked = selectionPolicy.rankCandidates(candidates, viewportScaleDenominator);

  QuiltSelectionResult selection;
  selection.viewportScaleDenominator = viewportScaleDenominator;
  selection.candidateCount = candidates.size();
  selection.orderedChartIds.reserve(ranked.size());

  std::vector<projection::ProjectedExtent> ownedRegions;
  std::uint32_t drawOrder = 0;
  for(const auto *entry : ranked) {
    if(entry == nullptr) {
      continue;
    }

    projection::ProjectedExtent projectedFullExtent{};
    if(!projectionContext.projectExtent(entry->extent, projectedFullExtent)) {
      continue;
    }

    const auto projectedVisibleExtent =
      intersectProjectedExtents(projectedFullExtent, projectedViewportExtent);
    if(!hasPositiveArea(projectedVisibleExtent)) {
      continue;
    }

    const auto visibleExtent = intersectExtents(entry->extent, viewportExtent);
    if(!visibleExtent.isValid()) {
      continue;
    }

    auto projectedPatchExtents =
      clipPatchesAgainstOwnedRegions({projectedVisibleExtent}, ownedRegions);
    if(projectedPatchExtents.empty()) {
      continue;
    }

    selection.orderedChartIds.push_back(entry->id);
    auto layer = QuiltLayer::fromCatalogEntry(*entry, drawOrder, visibleExtent);
    layer.projectedFullExtent = projectedFullExtent;
    layer.projectedVisibleExtent = projectedVisibleExtent;
    layer.projectedPatchExtents = projectedPatchExtents;
    plan.addLayer(std::move(layer));

    ownedRegions.insert(
      ownedRegions.end(),
      projectedPatchExtents.begin(),
      projectedPatchExtents.end());
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

projection::ProjectedExtent QuiltPlanner::intersectProjectedExtents(
  const projection::ProjectedExtent &lhs,
  const projection::ProjectedExtent &rhs) noexcept
{
  return {
    std::max(lhs.minX, rhs.minX),
    std::max(lhs.minY, rhs.minY),
    std::min(lhs.maxX, rhs.maxX),
    std::min(lhs.maxY, rhs.maxY)};
}

}// namespace chart_view::runtime::quilt
