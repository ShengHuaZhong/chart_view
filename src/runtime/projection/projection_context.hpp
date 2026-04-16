#ifndef CHART_VIEW_RUNTIME_PROJECTION_PROJECTION_CONTEXT_HPP
#define CHART_VIEW_RUNTIME_PROJECTION_PROJECTION_CONTEXT_HPP

#include "../chart_data/geometry.hpp"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace chart_view::runtime::projection {

struct ProjectedPoint
{
  double x{0.0};
  double y{0.0};

  [[nodiscard]] bool isFinite() const noexcept;
};

struct ProjectedExtent
{
  double minX{0.0};
  double minY{0.0};
  double maxX{0.0};
  double maxY{0.0};

  [[nodiscard]] bool isValid() const noexcept;
};

class ProjectionContext
{
public:
  static constexpr std::string_view kSourceGeographicCrs =
    "+proj=longlat +datum=WGS84 +no_defs +type=crs";
  static constexpr std::string_view kDisplayMercatorCrs =
    "+proj=merc +lon_0=0 +k=1 +x_0=0 +y_0=0 +datum=WGS84 +units=m +no_defs +type=crs";

  ProjectionContext() = default;
  ~ProjectionContext();

  ProjectionContext(ProjectionContext &&other) noexcept;
  ProjectionContext &operator=(ProjectionContext &&other) noexcept;

  ProjectionContext(const ProjectionContext &) = delete;
  ProjectionContext &operator=(const ProjectionContext &) = delete;

  [[nodiscard]] static ProjectionContext createMercator();
  [[nodiscard]] static ProjectionContext create(std::string_view displayCrsDefinition);

  [[nodiscard]] bool isValid() const noexcept;
  [[nodiscard]] std::string_view displayCrs() const noexcept;
  [[nodiscard]] const std::string &lastError() const noexcept;

  [[nodiscard]] bool project(
    const chart_data::Coordinate &source,
    ProjectedPoint &out) const noexcept;
  [[nodiscard]] bool unproject(
    const ProjectedPoint &source,
    chart_data::Coordinate &out) const noexcept;
  [[nodiscard]] bool projectPoints(
    std::span<const chart_data::Coordinate> source,
    std::vector<ProjectedPoint> &out) const noexcept;
  [[nodiscard]] bool projectExtent(
    const chart_data::Extent &source,
    ProjectedExtent &out) const noexcept;

private:
  struct Impl;

  explicit ProjectionContext(
    std::unique_ptr<Impl> impl,
    std::string displayCrsDefinition,
    std::string errorMessage);

  std::unique_ptr<Impl> m_impl;
  std::string m_displayCrsDefinition;
  std::string m_lastError;
};

}// namespace chart_view::runtime::projection

#endif
