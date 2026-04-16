#ifndef CHART_VIEW_RUNTIME_PROJECTION_PROJECTED_BOUNDS_HPP
#define CHART_VIEW_RUNTIME_PROJECTION_PROJECTED_BOUNDS_HPP

#include "projected_viewport.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <type_traits>
#include <variant>

namespace chart_view::runtime::projection {

namespace detail {

inline bool expandProjectedExtent(
  const ProjectedPoint &point,
  ProjectedExtent &extent,
  bool &havePoint) noexcept
{
  if(!point.isFinite()) {
    return false;
  }

  if(!havePoint) {
    extent = {point.x, point.y, point.x, point.y};
    havePoint = true;
    return true;
  }

  extent.minX = std::min(extent.minX, point.x);
  extent.minY = std::min(extent.minY, point.y);
  extent.maxX = std::max(extent.maxX, point.x);
  extent.maxY = std::max(extent.maxY, point.y);
  return true;
}

inline bool projectCoordinateRangeExtent(
  const ProjectionContext &projectionContext,
  std::span<const chart_data::Coordinate> coordinates,
  ProjectedExtent &out) noexcept
{
  bool havePoint = false;
  ProjectedExtent extent{};

  for(const auto &coordinate : coordinates) {
    ProjectedPoint projected{};
    if(!projectionContext.project(coordinate, projected)) {
      return false;
    }

    if(!expandProjectedExtent(projected, extent, havePoint)) {
      return false;
    }
  }

  if(!havePoint || !extent.isValid()) {
    return false;
  }

  out = extent;
  return true;
}

}// namespace detail

inline bool projectGeometryExtent(
  const ProjectionContext &projectionContext,
  const chart_data::Geometry &geometry,
  ProjectedExtent &out) noexcept
{
  bool havePoint = false;
  ProjectedExtent extent{};
  bool ok = true;

  std::visit(
    [&](auto &&value) {
      using T = std::decay_t<decltype(value)>;

      if constexpr(std::is_same_v<T, chart_data::PointGeometry>) {
        ProjectedPoint projected{};
        ok = projectionContext.project(value.position, projected)
          && detail::expandProjectedExtent(projected, extent, havePoint);
      } else if constexpr(std::is_same_v<T, chart_data::LineGeometry>) {
        ok = detail::projectCoordinateRangeExtent(projectionContext, value.vertices, extent);
        havePoint = ok;
      } else if constexpr(std::is_same_v<T, chart_data::AreaGeometry>) {
        ok = detail::projectCoordinateRangeExtent(projectionContext, value.exteriorRing, extent);
        if(!ok) {
          return;
        }

        havePoint = true;
        for(const auto &hole : value.interiorRings) {
          if(hole.empty()) {
            continue;
          }

          ProjectedExtent holeExtent{};
          ok = detail::projectCoordinateRangeExtent(projectionContext, hole, holeExtent);
          if(!ok) {
            return;
          }

          extent.minX = std::min(extent.minX, holeExtent.minX);
          extent.minY = std::min(extent.minY, holeExtent.minY);
          extent.maxX = std::max(extent.maxX, holeExtent.maxX);
          extent.maxY = std::max(extent.maxY, holeExtent.maxY);
        }
      }
    },
    geometry);

  if(!ok || !havePoint || !extent.isValid()) {
    return false;
  }

  out = extent;
  return true;
}

inline bool projectViewportBounds(
  const chart_view_viewport_t &viewport,
  const ProjectionContext &projectionContext,
  ProjectedExtent &out) noexcept
{
  ProjectedViewport projectedViewport;
  if(!ProjectedViewport::create(viewport, projectionContext, projectedViewport)
     || !projectedViewport.bounds.isValid()) {
    return false;
  }

  out = projectedViewport.bounds;
  return true;
}

inline bool computeViewportGeographicExtent(
  const chart_view_viewport_t &viewport,
  const ProjectionContext &projectionContext,
  chart_data::Extent &out) noexcept
{
  ProjectedExtent projectedViewport{};
  if(!projectViewportBounds(viewport, projectionContext, projectedViewport)) {
    return false;
  }

  const std::array<ProjectedPoint, 4> corners{{
    {projectedViewport.minX, projectedViewport.minY},
    {projectedViewport.minX, projectedViewport.maxY},
    {projectedViewport.maxX, projectedViewport.minY},
    {projectedViewport.maxX, projectedViewport.maxY},
  }};

  bool havePoint = false;
  chart_data::Extent extent{};
  for(const auto &corner : corners) {
    chart_data::Coordinate geographic{};
    if(!projectionContext.unproject(corner, geographic)) {
      return false;
    }

    if(!havePoint) {
      extent = {geographic.lon, geographic.lat, geographic.lon, geographic.lat};
      havePoint = true;
      continue;
    }

    extent.minLon = std::min(extent.minLon, geographic.lon);
    extent.minLat = std::min(extent.minLat, geographic.lat);
    extent.maxLon = std::max(extent.maxLon, geographic.lon);
    extent.maxLat = std::max(extent.maxLat, geographic.lat);
  }

  if(!havePoint || !extent.isValid()) {
    return false;
  }

  out = extent;
  return true;
}

inline bool projectedExtentsOverlap(
  const ProjectedExtent &lhs,
  const ProjectedExtent &rhs) noexcept
{
  return lhs.isValid() && rhs.isValid() && lhs.minX <= rhs.maxX && lhs.maxX >= rhs.minX
      && lhs.minY <= rhs.maxY && lhs.maxY >= rhs.minY;
}

}// namespace chart_view::runtime::projection

#endif
