#include "projection_context.hpp"

#include <proj.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace chart_view::runtime::projection {

namespace {

constexpr std::string_view kDisplayMercatorPipeline =
  "+proj=pipeline +step +proj=unitconvert +xy_in=deg +xy_out=rad "
  "+step +proj=merc +lon_0=0 +k=1 +x_0=0 +y_0=0 "
  "+a=6378137 +rf=298.257223563";

[[nodiscard]] bool isFiniteValue(double value) noexcept
{
  return std::isfinite(value);
}

[[nodiscard]] bool isFiniteCoord(const PJ_COORD &coord) noexcept
{
  return isFiniteValue(coord.xy.x) && isFiniteValue(coord.xy.y);
}

}// namespace

struct ProjectionContext::Impl
{
  PJ_CONTEXT *context{nullptr};
  PJ *transform{nullptr};

  ~Impl()
  {
    if(transform != nullptr) {
      proj_destroy(transform);
    }
    if(context != nullptr) {
      proj_context_destroy(context);
    }
  }
};

bool ProjectedPoint::isFinite() const noexcept
{
  return isFiniteValue(x) && isFiniteValue(y);
}

bool ProjectedExtent::isValid() const noexcept
{
  return minX <= maxX && minY <= maxY && isFiniteValue(minX) && isFiniteValue(minY)
      && isFiniteValue(maxX) && isFiniteValue(maxY);
}

ProjectionContext::ProjectionContext(
  std::unique_ptr<Impl> impl,
  std::string displayCrsDefinition,
  std::string errorMessage)
  : m_impl(std::move(impl))
  , m_displayCrsDefinition(std::move(displayCrsDefinition))
  , m_lastError(std::move(errorMessage))
{
}

ProjectionContext::~ProjectionContext() = default;

ProjectionContext::ProjectionContext(ProjectionContext &&other) noexcept = default;

ProjectionContext &ProjectionContext::operator=(ProjectionContext &&other) noexcept = default;

ProjectionContext ProjectionContext::createMercator()
{
  return create(kDisplayMercatorCrs);
}

ProjectionContext ProjectionContext::create(std::string_view displayCrsDefinition)
{
  if(displayCrsDefinition.empty()) {
    return ProjectionContext(nullptr, {}, "display CRS definition is empty");
  }

  auto impl = std::make_unique<Impl>();
  impl->context = proj_context_create();
  if(impl->context == nullptr) {
    return ProjectionContext(nullptr, {}, "proj_context_create failed");
  }

  const std::string sourceCrsDefinition(kSourceGeographicCrs);
  const std::string displayCrs(displayCrsDefinition);
  PJ *rawTransform = nullptr;
  if(displayCrsDefinition == ProjectionContext::kDisplayMercatorCrs) {
    rawTransform = proj_create(impl->context, std::string(kDisplayMercatorPipeline).c_str());
  } else {
    rawTransform = proj_create_crs_to_crs(
      impl->context,
      sourceCrsDefinition.c_str(),
      displayCrs.c_str(),
      nullptr);
  }
  if(rawTransform == nullptr) {
    return ProjectionContext(nullptr, {}, "proj transform creation failed");
  }

  if(displayCrsDefinition == ProjectionContext::kDisplayMercatorCrs) {
    impl->transform = rawTransform;
  } else {
    impl->transform = proj_normalize_for_visualization(impl->context, rawTransform);
    proj_destroy(rawTransform);
    if(impl->transform == nullptr) {
      return ProjectionContext(nullptr, {}, "proj_normalize_for_visualization failed");
    }
  }

  return ProjectionContext(std::move(impl), displayCrs, {});
}

bool ProjectionContext::isValid() const noexcept
{
  return m_impl != nullptr && m_impl->context != nullptr && m_impl->transform != nullptr;
}

std::string_view ProjectionContext::displayCrs() const noexcept
{
  return m_displayCrsDefinition;
}

const std::string &ProjectionContext::lastError() const noexcept
{
  return m_lastError;
}

bool ProjectionContext::project(
  const chart_data::Coordinate &source,
  ProjectedPoint &out) const noexcept
{
  if(!isValid() || !isFiniteValue(source.lon) || !isFiniteValue(source.lat)) {
    return false;
  }

  const auto input = proj_coord(source.lon, source.lat, 0.0, 0.0);
  const auto projected = proj_trans(m_impl->transform, PJ_FWD, input);
  if(!isFiniteCoord(projected)) {
    return false;
  }

  out = {projected.xy.x, projected.xy.y};
  return true;
}

bool ProjectionContext::unproject(
  const ProjectedPoint &source,
  chart_data::Coordinate &out) const noexcept
{
  if(!isValid() || !source.isFinite()) {
    return false;
  }

  const auto input = proj_coord(source.x, source.y, 0.0, 0.0);
  const auto geographic = proj_trans(m_impl->transform, PJ_INV, input);
  if(!isFiniteCoord(geographic)) {
    return false;
  }

  out = {geographic.lp.lam, geographic.lp.phi};
  return true;
}

bool ProjectionContext::projectPoints(
  std::span<const chart_data::Coordinate> source,
  std::vector<ProjectedPoint> &out) const noexcept
{
  out.clear();
  out.reserve(source.size());

  for(const auto &coord : source) {
    ProjectedPoint projected{};
    if(!project(coord, projected)) {
      out.clear();
      return false;
    }
    out.push_back(projected);
  }

  return true;
}

bool ProjectionContext::projectExtent(
  const chart_data::Extent &source,
  ProjectedExtent &out) const noexcept
{
  if(!source.isValid()) {
    return false;
  }

  const std::array<chart_data::Coordinate, 4> corners{{
    {source.minLon, source.minLat},
    {source.minLon, source.maxLat},
    {source.maxLon, source.minLat},
    {source.maxLon, source.maxLat},
  }};

  std::vector<ProjectedPoint> projectedCorners;
  if(!projectPoints(corners, projectedCorners) || projectedCorners.empty()) {
    return false;
  }

  out = {
    projectedCorners.front().x,
    projectedCorners.front().y,
    projectedCorners.front().x,
    projectedCorners.front().y};

  for(const auto &corner : projectedCorners) {
    out.minX = std::min(out.minX, corner.x);
    out.minY = std::min(out.minY, corner.y);
    out.maxX = std::max(out.maxX, corner.x);
    out.maxY = std::max(out.maxY, corner.y);
  }

  return out.isValid();
}

}// namespace chart_view::runtime::projection
