#ifndef CHART_VIEW_RUNTIME_QUILT_QUILT_PLANNER_HPP
#define CHART_VIEW_RUNTIME_QUILT_QUILT_PLANNER_HPP

#include "../catalog/chart_selection_policy.hpp"
#include "../catalog/coverage_index.hpp"
#include "../projection/projection_context.hpp"
#include "quilt_plan.hpp"

namespace chart_view::runtime::quilt {

class QuiltPlanner
{
public:
  QuiltPlanner() = default;

  [[nodiscard]] QuiltPlan build(
    const catalog::CoverageIndex &coverageIndex,
    const catalog::ChartSelectionPolicy &selectionPolicy,
    chart_data::Extent viewportExtent,
    double viewportScaleDenominator) const;

private:
  [[nodiscard]] static chart_data::Extent intersectExtents(
    const chart_data::Extent &lhs,
    const chart_data::Extent &rhs) noexcept;
  [[nodiscard]] static projection::ProjectedExtent intersectProjectedExtents(
    const projection::ProjectedExtent &lhs,
    const projection::ProjectedExtent &rhs) noexcept;
};

}// namespace chart_view::runtime::quilt

#endif
