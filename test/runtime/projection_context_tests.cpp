#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "projection/projection_context.hpp"
#include "projection/projected_viewport.hpp"

#include <array>
#include <vector>

namespace {
constexpr double kLonTolerance = 1e-6;
constexpr double kLatTolerance = 1e-6;
}

TEST_CASE("ProjectionContext creates a runtime-owned mercator transform", "[projection]")
{
  const auto projectionContext = chart_view::runtime::projection::ProjectionContext::createMercator();

  REQUIRE(projectionContext.isValid());
  REQUIRE(projectionContext.displayCrs()
          == chart_view::runtime::projection::ProjectionContext::kDisplayMercatorCrs);
  REQUIRE(projectionContext.lastError().empty());
}

TEST_CASE("ProjectionContext round-trips lon lat through display projection", "[projection]")
{
  const auto projectionContext = chart_view::runtime::projection::ProjectionContext::createMercator();
  REQUIRE(projectionContext.isValid());

  const chart_view::runtime::chart_data::Coordinate source{121.5, 31.2};
  chart_view::runtime::projection::ProjectedPoint projected{};
  REQUIRE(projectionContext.project(source, projected));
  REQUIRE(projected.isFinite());

  chart_view::runtime::chart_data::Coordinate restored{};
  REQUIRE(projectionContext.unproject(projected, restored));
  REQUIRE(restored.lon == Catch::Approx(source.lon).margin(kLonTolerance));
  REQUIRE(restored.lat == Catch::Approx(source.lat).margin(kLatTolerance));
}

TEST_CASE("ProjectionContext projects point batches and extents", "[projection]")
{
  using chart_view::runtime::chart_data::Coordinate;
  using chart_view::runtime::chart_data::Extent;
  using chart_view::runtime::projection::ProjectedExtent;
  using chart_view::runtime::projection::ProjectedPoint;

  const auto projectionContext = chart_view::runtime::projection::ProjectionContext::createMercator();
  REQUIRE(projectionContext.isValid());

  const std::array<Coordinate, 3> points{{
    {-0.1, 50.9},
    {0.0, 51.0},
    {0.1, 51.1},
  }};

  std::vector<ProjectedPoint> projectedPoints;
  REQUIRE(projectionContext.projectPoints(points, projectedPoints));
  REQUIRE(projectedPoints.size() == points.size());
  REQUIRE(projectedPoints[0].x < projectedPoints[1].x);
  REQUIRE(projectedPoints[1].x < projectedPoints[2].x);
  REQUIRE(projectedPoints[0].y < projectedPoints[1].y);
  REQUIRE(projectedPoints[1].y < projectedPoints[2].y);

  ProjectedExtent projectedExtent{};
  REQUIRE(projectionContext.projectExtent(Extent{-0.1, 50.9, 0.1, 51.1}, projectedExtent));
  REQUIRE(projectedExtent.isValid());
  REQUIRE(projectedExtent.minX <= projectedPoints.front().x);
  REQUIRE(projectedExtent.maxX >= projectedPoints.back().x);
}

TEST_CASE("ProjectionContext rejects invalid display CRS definitions", "[projection]")
{
  const auto invalidContext = chart_view::runtime::projection::ProjectionContext::create({});

  REQUIRE_FALSE(invalidContext.isValid());
  REQUIRE_FALSE(invalidContext.lastError().empty());
}

TEST_CASE("ProjectedViewport maps the display projection to pixel space", "[projection][viewport]")
{
  const auto projectionContext = chart_view::runtime::projection::ProjectionContext::createMercator();
  REQUIRE(projectionContext.isValid());

  chart_view_viewport_t viewport{};
  viewport.center_lon = 0.0;
  viewport.center_lat = 51.0;
  viewport.scale_denominator = 100000.0;
  viewport.pixel_width = 800;
  viewport.pixel_height = 600;

  chart_view::runtime::projection::ProjectedViewport projectedViewport;
  REQUIRE(chart_view::runtime::projection::ProjectedViewport::create(
    viewport,
    projectionContext,
    projectedViewport));
  REQUIRE(projectedViewport.isValid());

  chart_view::runtime::SurfacePoint centerPixel{};
  REQUIRE(projectedViewport.projectToPixel(
    projectionContext,
    {viewport.center_lon, viewport.center_lat},
    centerPixel));
  REQUIRE(centerPixel.x == 400);
  REQUIRE(centerPixel.y == 300);

  chart_view::runtime::SurfacePoint eastPixel{};
  REQUIRE(projectedViewport.projectToPixel(projectionContext, {0.02, 51.0}, eastPixel));
  REQUIRE(eastPixel.x > centerPixel.x);

  chart_view::runtime::chart_data::Coordinate restoredCenter{};
  REQUIRE(projectedViewport.unprojectFromPixel(
    projectionContext,
    centerPixel,
    restoredCenter));
  REQUIRE(restoredCenter.lon == Catch::Approx(viewport.center_lon).margin(kLonTolerance));
  REQUIRE(restoredCenter.lat == Catch::Approx(viewport.center_lat).margin(kLatTolerance));
}

TEST_CASE("ProjectedViewport rejects invalid public viewport input", "[projection][viewport]")
{
  const auto projectionContext = chart_view::runtime::projection::ProjectionContext::createMercator();
  REQUIRE(projectionContext.isValid());

  chart_view_viewport_t viewport{};
  viewport.center_lon = 0.0;
  viewport.center_lat = 51.0;
  viewport.pixel_width = 800;
  viewport.pixel_height = 600;

  chart_view::runtime::projection::ProjectedViewport projectedViewport;
  REQUIRE_FALSE(chart_view::runtime::projection::ProjectedViewport::create(
    viewport,
    projectionContext,
    projectedViewport));
}
