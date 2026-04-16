#ifndef CHART_VIEW_RUNTIME_CATALOG_CHART_SELECTION_POLICY_HPP
#define CHART_VIEW_RUNTIME_CATALOG_CHART_SELECTION_POLICY_HPP

#include "chart_catalog.hpp"

#include <cstdint>
#include <vector>

namespace chart_view::runtime::catalog {

class ChartSelectionPolicy
{
public:
  ChartSelectionPolicy() = default;

  [[nodiscard]] std::vector<const ChartCatalogEntry *> rankCandidates(
    const std::vector<const ChartCatalogEntry *> &candidates,
    double viewportScaleDenominator) const;

private:
  [[nodiscard]] std::uint32_t targetUsageBand(double viewportScaleDenominator) const noexcept;
  [[nodiscard]] int coarsenessPenalty(
    const ChartCatalogEntry &entry,
    double viewportScaleDenominator) const noexcept;
  [[nodiscard]] double scaleDistance(
    const ChartCatalogEntry &entry,
    double viewportScaleDenominator) const noexcept;
  [[nodiscard]] int usagePenalty(
    const ChartCatalogEntry &entry,
    double viewportScaleDenominator) const noexcept;
  [[nodiscard]] int sourcePriority(const ChartCatalogEntry &entry) const noexcept;
};

}// namespace chart_view::runtime::catalog

#endif
