#include <catch2/catch_test_macros.hpp>

#include "feature_layer_renderer.hpp"
#include "scene_builder_from_senc.hpp"
#include "rhi_render_backend.hpp"
#include "chart_data/feature_chart_dataset.hpp"
#include "chart_data/geometry.hpp"

#include <QGuiApplication>

#include <array>
#include <ranges>
#include <span>
#include <vector>

namespace {
struct AppGuard
{
  static int argc;
  static char *argv[];
  QGuiApplication app{argc, argv};
};
int AppGuard::argc = 1;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
char *AppGuard::argv[] = {const_cast<char *>("feature_renderer_tests")};

chart_view::runtime::chart_data::FeatureChartDataset makeDataset()
{
  using namespace chart_view::runtime::chart_data;

  FeatureChartDataset ds;
  DatasetMeta meta;
  meta.name = "render_test";
  meta.sourceType = chart_view_chart_source_s57;
  meta.extent = {-1.0, 50.0, 1.0, 52.0};
  ds.setMeta(std::move(meta));

  // Point
  Feature f0;
  f0.id = 1;
  f0.classCode = 100;
  f0.geometry = PointGeometry{{0.0, 51.0}};
  ds.addFeature(std::move(f0));

  // Line
  Feature f1;
  f1.id = 2;
  f1.classCode = 200;
  f1.geometry = LineGeometry{{{-0.5, 50.5}, {0.5, 51.5}}};
  ds.addFeature(std::move(f1));

  // Area
  Feature f2;
  f2.id = 3;
  f2.classCode = 300;
  f2.geometry = AreaGeometry{{{-0.5, 50.0}, {0.5, 50.0}, {0.5, 51.0}, {-0.5, 51.0}}, {}};
  ds.addFeature(std::move(f2));

  return ds;
}

chart_view::runtime::ViewportState makeViewport()
{
  chart_view::runtime::ViewportState vs;
  chart_view_viewport_t vp{};
  vp.center_lon = 0.0;
  vp.center_lat = 51.0;
  vp.scale_denominator = 100000.0;
  vp.pixel_width = 800;
  vp.pixel_height = 600;
  vs.set(vp);
  return vs;
}
}// namespace

TEST_CASE("FeatureLayerRenderer rejects uninitialized backend", "[renderer][rhi]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;

  auto ds = makeDataset();
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);

  chart_view::runtime::FeatureLayerRenderer renderer;
  auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_not_initialized);
}

TEST_CASE("FeatureLayerRenderer renders empty snapshot", "[renderer][rhi]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  chart_view::runtime::chart_data::FeatureChartDataset emptyDs;
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.build(emptyDs, vs);

  chart_view::runtime::FeatureLayerRenderer renderer;
  auto result = renderer.render(*snap, emptyDs, backend);
  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.totalVertices == 0);
}

TEST_CASE("FeatureLayerRenderer processes all geometry types", "[renderer][rhi]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  auto ds = makeDataset();
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);

  chart_view::runtime::FeatureLayerRenderer renderer;
  auto result = renderer.render(*snap, ds, backend);

  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.pointsRendered == 1);
  REQUIRE(result.linesRendered == 1);
  REQUIRE(result.areasRendered == 1);
  // 1 point vertex + 2 line vertices + 4 area vertices = 7
  REQUIRE(result.totalVertices == 7);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const std::array<std::uint8_t, 4> background{230U, 230U, 217U, 255U};
  const auto nonBackgroundPixel = std::ranges::any_of(
    std::views::iota(std::size_t{0}, rgba.size() / 4U),
    [&](std::size_t pixelIndex) {
      const auto offset = pixelIndex * 4U;
      return rgba[offset + 0] != background[0]
          || rgba[offset + 1] != background[1]
          || rgba[offset + 2] != background[2]
          || rgba[offset + 3] != background[3];
    });
  REQUIRE(nonBackgroundPixel);
}

TEST_CASE("FeatureLayerRenderer viewport culling reduces output", "[renderer][rhi]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  auto ds = makeDataset();

  // Add a far-away feature that should be culled.
  chart_view::runtime::chart_data::Feature farAway;
  farAway.id = 99;
  farAway.classCode = 999;
  farAway.geometry = chart_view::runtime::chart_data::PointGeometry{{120.0, -30.0}};
  ds.addFeature(std::move(farAway));

  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  // build() with viewport culling
  auto snap = builder.build(ds, vs);

  chart_view::runtime::FeatureLayerRenderer renderer;
  auto result = renderer.render(*snap, ds, backend);

  REQUIRE(result.status == chart_view_status_ok);
  // Far-away point should be culled, so we expect 3 features not 4.
  REQUIRE(result.pointsRendered == 1);
  REQUIRE(result.linesRendered == 1);
  REQUIRE(result.areasRendered == 1);
}
