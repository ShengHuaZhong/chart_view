#ifndef CHART_VIEW_RUNTIME_PROJECTION_PROJECTED_VIEWPORT_HPP
#define CHART_VIEW_RUNTIME_PROJECTION_PROJECTED_VIEWPORT_HPP

#include "projection_context.hpp"

#include "../render_types.hpp"

#include <chart_view/runtime/chart_runtime_types.h>

namespace chart_view::runtime::projection {

struct ProjectedViewport
{
  ProjectedPoint center;
  ProjectedExtent bounds;
  double metresPerPixel{0.0};
  int pixelWidth{0};
  int pixelHeight{0};

  [[nodiscard]] bool isValid() const noexcept;

  [[nodiscard]] static bool create(
    const chart_view_viewport_t &viewport,
    const ProjectionContext &projectionContext,
    ProjectedViewport &out) noexcept;

  [[nodiscard]] bool projectToPixel(
    const ProjectionContext &projectionContext,
    const chart_data::Coordinate &coord,
    SurfacePoint &out) const noexcept;
  [[nodiscard]] bool unprojectFromPixel(
    const ProjectionContext &projectionContext,
    const SurfacePoint &point,
    chart_data::Coordinate &out) const noexcept;
};

}// namespace chart_view::runtime::projection

#endif
