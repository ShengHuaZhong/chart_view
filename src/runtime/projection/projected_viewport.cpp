#include "projected_viewport.hpp"

#include <algorithm>
#include <cmath>

namespace chart_view::runtime::projection {

namespace {

constexpr double kPixelsPerMetre = 3779.5275591;

[[nodiscard]] bool isFinite(double value) noexcept
{
  return std::isfinite(value);
}

}// namespace

bool ProjectedViewport::isValid() const noexcept
{
  return center.isFinite() && bounds.isValid() && metresPerPixel > 0.0 && pixelWidth > 0
      && pixelHeight > 0;
}

bool ProjectedViewport::create(
  const chart_view_viewport_t &viewport,
  const ProjectionContext &projectionContext,
  ProjectedViewport &out) noexcept
{
  if(viewport.pixel_width <= 0 || viewport.pixel_height <= 0 || viewport.scale_denominator <= 0.0) {
    return false;
  }

  ProjectedPoint projectedCenter{};
  if(!projectionContext.project(
       {viewport.center_lon, viewport.center_lat},
       projectedCenter)) {
    return false;
  }

  const auto metresPerPixel = viewport.scale_denominator / kPixelsPerMetre;
  if(!isFinite(metresPerPixel) || metresPerPixel <= 0.0) {
    return false;
  }

  const auto safeWidth = std::max(viewport.pixel_width, 1);
  const auto safeHeight = std::max(viewport.pixel_height, 1);
  const auto halfWidth = static_cast<double>(safeWidth) * 0.5 * metresPerPixel;
  const auto halfHeight = static_cast<double>(safeHeight) * 0.5 * metresPerPixel;

  out.center = projectedCenter;
  out.metresPerPixel = metresPerPixel;
  out.pixelWidth = safeWidth;
  out.pixelHeight = safeHeight;
  out.bounds = {
    projectedCenter.x - halfWidth,
    projectedCenter.y - halfHeight,
    projectedCenter.x + halfWidth,
    projectedCenter.y + halfHeight};
  return out.isValid();
}

bool ProjectedViewport::projectToPixel(
  const ProjectionContext &projectionContext,
  const chart_data::Coordinate &coord,
  SurfacePoint &out) const noexcept
{
  if(!isValid()) {
    return false;
  }

  ProjectedPoint projected{};
  if(!projectionContext.project(coord, projected)) {
    return false;
  }

  const auto pixelX =
    static_cast<double>(pixelWidth) * 0.5 + (projected.x - center.x) / metresPerPixel;
  const auto pixelY =
    static_cast<double>(pixelHeight) * 0.5 - (projected.y - center.y) / metresPerPixel;
  if(!isFinite(pixelX) || !isFinite(pixelY)) {
    return false;
  }

  out = {
    static_cast<int>(std::lround(pixelX)),
    static_cast<int>(std::lround(pixelY))};
  return true;
}

bool ProjectedViewport::unprojectFromPixel(
  const ProjectionContext &projectionContext,
  const SurfacePoint &point,
  chart_data::Coordinate &out) const noexcept
{
  if(!isValid()) {
    return false;
  }

  const auto projectedX =
    center.x + (static_cast<double>(point.x) - static_cast<double>(pixelWidth) * 0.5) * metresPerPixel;
  const auto projectedY =
    center.y - (static_cast<double>(point.y) - static_cast<double>(pixelHeight) * 0.5) * metresPerPixel;
  if(!isFinite(projectedX) || !isFinite(projectedY)) {
    return false;
  }

  return projectionContext.unproject({projectedX, projectedY}, out);
}

}// namespace chart_view::runtime::projection
