#include "chart_selection_policy.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <tuple>

namespace chart_view::runtime::catalog {

namespace {

constexpr double kScoreEpsilon = 1e-9;

std::string sortKey(const ChartCatalogEntry &entry)
{
  return entry.id + "|" + entry.sencPath.generic_string();
}

}// namespace

std::vector<const ChartCatalogEntry *> ChartSelectionPolicy::rankCandidates(
  const std::vector<const ChartCatalogEntry *> &candidates,
  double viewportScaleDenominator) const
{
  std::vector<const ChartCatalogEntry *> ranked;
  ranked.reserve(candidates.size());
  for(const auto *candidate : candidates) {
    if(candidate != nullptr) {
      ranked.push_back(candidate);
    }
  }

  std::stable_sort(
    ranked.begin(),
    ranked.end(),
    [&](const ChartCatalogEntry *lhs, const ChartCatalogEntry *rhs) {
      const auto lhsScaleDistance = scaleDistance(*lhs, viewportScaleDenominator);
      const auto rhsScaleDistance = scaleDistance(*rhs, viewportScaleDenominator);
      if(std::abs(lhsScaleDistance - rhsScaleDistance) > kScoreEpsilon) {
        return lhsScaleDistance < rhsScaleDistance;
      }

      const auto lhsUsagePenalty = usagePenalty(*lhs, viewportScaleDenominator);
      const auto rhsUsagePenalty = usagePenalty(*rhs, viewportScaleDenominator);
      if(lhsUsagePenalty != rhsUsagePenalty) {
        return lhsUsagePenalty < rhsUsagePenalty;
      }

      const auto lhsSourcePriority = sourcePriority(*lhs);
      const auto rhsSourcePriority = sourcePriority(*rhs);
      if(lhsSourcePriority != rhsSourcePriority) {
        return lhsSourcePriority < rhsSourcePriority;
      }

      return sortKey(*lhs) < sortKey(*rhs);
    });

  return ranked;
}

std::uint32_t ChartSelectionPolicy::targetUsageBand(double viewportScaleDenominator) const noexcept
{
  if(viewportScaleDenominator <= 0.0) {
    return 0;
  }
  if(viewportScaleDenominator <= 22000.0) {
    return 6;
  }
  if(viewportScaleDenominator <= 90000.0) {
    return 5;
  }
  if(viewportScaleDenominator <= 350000.0) {
    return 4;
  }
  if(viewportScaleDenominator <= 1500000.0) {
    return 3;
  }
  if(viewportScaleDenominator <= 4000000.0) {
    return 2;
  }
  return 1;
}

double ChartSelectionPolicy::scaleDistance(
  const ChartCatalogEntry &entry,
  double viewportScaleDenominator) const noexcept
{
  if(entry.nativeScale <= 0.0 || viewportScaleDenominator <= 0.0) {
    return std::numeric_limits<double>::max();
  }

  return std::abs(std::log(entry.nativeScale / viewportScaleDenominator));
}

int ChartSelectionPolicy::usagePenalty(
  const ChartCatalogEntry &entry,
  double viewportScaleDenominator) const noexcept
{
  if(entry.usageBand == 0) {
    return std::numeric_limits<int>::max();
  }

  return std::abs(static_cast<int>(entry.usageBand) -
                  static_cast<int>(targetUsageBand(viewportScaleDenominator)));
}

int ChartSelectionPolicy::sourcePriority(const ChartCatalogEntry &entry) const noexcept
{
  switch(entry.sourceType) {
  case chart_view_chart_source_s101:
    return 0;
  case chart_view_chart_source_s57:
    return 1;
  case chart_view_chart_source_cm93:
    return 2;
  case chart_view_chart_source_unknown:
  default:
    return 3;
  }
}

}// namespace chart_view::runtime::catalog
