#ifndef CHART_VIEW_RUNTIME_CHART_DATA_GEOMETRY_HPP
#define CHART_VIEW_RUNTIME_CHART_DATA_GEOMETRY_HPP

#include <cstdint>
#include <variant>
#include <vector>

namespace chart_view::runtime::chart_data {

// 2-D coordinate (lon/lat in degrees).
struct Coordinate
{
  double lon{0.0};
  double lat{0.0};
};

// Axis-aligned bounding box.
struct Extent
{
  double minLon{0.0};
  double minLat{0.0};
  double maxLon{0.0};
  double maxLat{0.0};

  [[nodiscard]] bool isValid() const noexcept
  {
    return minLon <= maxLon && minLat <= maxLat;
  }
};

// Point geometry -- a single coordinate.
struct PointGeometry
{
  Coordinate position;
};

// Line geometry -- an ordered sequence of coordinates.
struct LineGeometry
{
  std::vector<Coordinate> vertices;
};

// Area geometry -- an exterior ring and optional interior rings (holes).
struct AreaGeometry
{
  std::vector<Coordinate> exteriorRing;
  std::vector<std::vector<Coordinate>> interiorRings;
};

// Variant holding any of the three geometry types.
using Geometry = std::variant<PointGeometry, LineGeometry, AreaGeometry>;

// Geometry type discriminator.
enum class GeometryType : std::uint8_t
{
  kPoint = 0,
  kLine = 1,
  kArea = 2
};

// Return the GeometryType for a Geometry variant.
[[nodiscard]] inline GeometryType geometryType(const Geometry &g) noexcept
{
  return static_cast<GeometryType>(g.index());
}

}// namespace chart_view::runtime::chart_data

#endif
