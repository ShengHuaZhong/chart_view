#include <catch2/catch_test_macros.hpp>

#include "feature_layer_renderer.hpp"
#include "scene_builder_from_senc.hpp"
#include "rhi_render_backend.hpp"
#include "chart_data/feature_chart_dataset.hpp"
#include "chart_data/geometry.hpp"
#include "quilt/quilt_plan.hpp"

#include <QGuiApplication>

#include <array>
#include <cstdint>
#include <limits>
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

chart_view::runtime::chart_data::FeatureChartDataset makeDataset(
  const char *name,
  chart_view_chart_source_type_t sourceType,
  const chart_view::runtime::chart_data::Extent &extent,
  std::vector<chart_view::runtime::chart_data::Feature> features)
{
  using namespace chart_view::runtime::chart_data;

  FeatureChartDataset ds;
  DatasetMeta meta;
  meta.name = name;
  meta.sourceType = sourceType;
  meta.extent = extent;
  ds.setMeta(std::move(meta));

  for(auto &feature : features) {
    ds.addFeature(std::move(feature));
  }

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

bool pixelMatches(
  std::span<const std::uint8_t> rgba,
  int width,
  int x,
  int y,
  const std::array<std::uint8_t, 4> &color)
{
  const auto offset = static_cast<std::size_t>((y * width + x) * 4);
  return offset + 3 < rgba.size() && rgba[offset + 0] == color[0] && rgba[offset + 1] == color[1]
      && rgba[offset + 2] == color[2] && rgba[offset + 3] == color[3];
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

TEST_CASE("FeatureLayerRenderer renders two charts in one frame", "[renderer][rhi][quilt]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature harborPoint;
  harborPoint.id = 1;
  harborPoint.classCode = 700;
  harborPoint.geometry = PointGeometry{{0.0, 51.0}};

  Feature approachArea;
  approachArea.id = 2;
  approachArea.classCode = 800;
  approachArea.geometry = AreaGeometry{{{-0.4, 50.6}, {0.4, 50.6}, {0.4, 51.4}, {-0.4, 51.4}}, {}};

  const auto harborDataset = makeDataset(
    "harbor",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {harborPoint});
  const auto approachDataset = makeDataset(
    "approach",
    chart_view_chart_source_s101,
    {-1.0, 50.0, 1.0, 52.0},
    {approachArea});

  chart_view::runtime::quilt::QuiltPlan plan;
  plan.setViewport({-1.0, 50.0, 1.0, 52.0}, 100000.0);
  plan.addLayer(chart_view::runtime::quilt::QuiltLayer{
    "harbor",
    "harbor.senc",
    chart_view_chart_source_s57,
    20000.0,
    5,
    0,
    {-1.0, 50.0, 1.0, 52.0},
    {-1.0, 50.0, 1.0, 52.0}});
  plan.addLayer(chart_view::runtime::quilt::QuiltLayer{
    "approach",
    "approach.senc",
    chart_view_chart_source_s101,
    90000.0,
    4,
    1,
    {-1.0, 50.0, 1.0, 52.0},
    {-1.0, 50.0, 1.0, 52.0}});

  auto vs = makeViewport();
  const std::array<FeatureChartDataset, 2> datasets{harborDataset, approachDataset};

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.build(plan, datasets, vs);
  REQUIRE(snap != nullptr);
  REQUIRE(snap->charts().size() == 2);

  chart_view::runtime::FeatureLayerRenderer renderer;
  auto result = renderer.render(*snap, std::span<const FeatureChartDataset>(datasets), backend);

  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.pointsRendered == 1);
  REQUIRE(result.linesRendered == 0);
  REQUIRE(result.areasRendered == 1);
  REQUIRE(result.totalVertices == 5);

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

TEST_CASE("FeatureLayerRenderer uses portrayal registry overrides", "[renderer][rhi][portrayal]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature point;
  point.id = 1;
  point.classCode = 129;
  point.classAcronym = "SOUNDG";
  point.geometry = PointGeometry{{0.0, 51.0}};
  point.attributes["VALSOU"] = 12.5;

  const auto ds = makeDataset(
    "portrayal_override",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {point});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::FeatureLayerRenderer renderer;
  renderer.portrayalRegistry().registerSymbolRuleForStyle(
    "point/sounding",
    chart_view::runtime::portrayal::SymbolRule{{12U, 200U, 45U, 255U}, 6});

  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.pointsRendered == 1);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const std::array<std::uint8_t, 4> customPointColor{12U, 200U, 45U, 255U};
  const auto foundCustomColor = std::ranges::any_of(
    std::views::iota(std::size_t{0}, rgba.size() / 4U),
    [&](std::size_t pixelIndex) {
      const auto offset = pixelIndex * 4U;
      return rgba[offset + 0] == customPointColor[0]
          && rgba[offset + 1] == customPointColor[1]
          && rgba[offset + 2] == customPointColor[2]
          && rgba[offset + 3] == customPointColor[3];
    });
  REQUIRE(foundCustomColor);
}

TEST_CASE("FeatureLayerRenderer renders sounding symbols instead of plain discs", "[renderer][rhi][portrayal][point_symbol]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature sounding;
  sounding.id = 1;
  sounding.classCode = 129;
  sounding.classAcronym = "SOUNDG";
  sounding.geometry = PointGeometry{{0.0, 51.0}};
  sounding.attributes["VALSOU"] = 12.5;

  const auto ds = makeDataset(
    "point_symbol",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {sounding});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::FeatureLayerRenderer renderer;
  renderer.portrayalRegistry().registerSymbolRuleForStyle(
    "point/sounding",
    chart_view::runtime::portrayal::SymbolRule{{12U, 200U, 45U, 255U}, 4});

  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.pointsRendered == 1);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const auto pixelMatches = [&](int x, int y, const std::array<std::uint8_t, 4> &color) {
    const auto offset = static_cast<std::size_t>((y * 800 + x) * 4);
    return offset + 3 < rgba.size() && rgba[offset + 0] == color[0] && rgba[offset + 1] == color[1]
        && rgba[offset + 2] == color[2] && rgba[offset + 3] == color[3];
  };

  const std::array<std::uint8_t, 4> symbolColor{12U, 200U, 45U, 255U};
  const std::array<std::uint8_t, 4> background{230U, 230U, 217U, 255U};
  REQUIRE(pixelMatches(400, 300, symbolColor));
  REQUIRE(pixelMatches(400, 297, symbolColor));
  REQUIRE(pixelMatches(397, 300, symbolColor));
  REQUIRE(pixelMatches(402, 302, background));
}

TEST_CASE("FeatureLayerRenderer renders depth contours with dashed line styles", "[renderer][rhi][portrayal][line_symbol]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature depthContour;
  depthContour.id = 1;
  depthContour.classCode = 42;
  depthContour.classAcronym = "DEPCNT";
  depthContour.geometry = LineGeometry{{{-0.12, 51.0}, {0.12, 51.0}}};
  depthContour.attributes["VALDCO"] = 20.0;

  const auto ds = makeDataset(
    "line_symbol",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {depthContour});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::FeatureLayerRenderer renderer;
  renderer.portrayalRegistry().registerLineStyleRuleForStyle(
    "line/depth_contour",
    chart_view::runtime::portrayal::LineStyleRule{{12U, 200U, 45U, 255U}, 2});

  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.linesRendered == 1);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const std::array<std::uint8_t, 4> lineColor{12U, 200U, 45U, 255U};
  const std::array<std::uint8_t, 4> background{230U, 230U, 217U, 255U};
  std::vector<int> coloredXs;
  coloredXs.reserve(800);
  for(int x = 0; x < 800; ++x) {
    const auto offset = static_cast<std::size_t>((300 * 800 + x) * 4);
    if(rgba[offset + 0] == lineColor[0] && rgba[offset + 1] == lineColor[1]
       && rgba[offset + 2] == lineColor[2] && rgba[offset + 3] == lineColor[3]) {
      coloredXs.push_back(x);
    }
  }

  REQUIRE_FALSE(coloredXs.empty());
  bool foundGapInsideStroke = false;
  for(int x = coloredXs.front(); x <= coloredXs.back(); ++x) {
    const auto offset = static_cast<std::size_t>((300 * 800 + x) * 4);
    const auto isBackground = rgba[offset + 0] == background[0] && rgba[offset + 1] == background[1]
        && rgba[offset + 2] == background[2] && rgba[offset + 3] == background[3];
    if(isBackground) {
      foundGapInsideStroke = true;
      break;
    }
  }
  REQUIRE(foundGapInsideStroke);
}

TEST_CASE("FeatureLayerRenderer renders depth areas with patterned fills", "[renderer][rhi][portrayal][area_symbol]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature depthArea;
  depthArea.id = 1;
  depthArea.classCode = 310;
  depthArea.classAcronym = "DEPARE";
  depthArea.geometry = AreaGeometry{{{-0.08, 50.92}, {0.08, 50.92}, {0.08, 51.08}, {-0.08, 51.08}}, {}};
  depthArea.attributes["DRVAL1"] = 5.0;

  const auto ds = makeDataset(
    "area_symbol",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {depthArea});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::FeatureLayerRenderer renderer;
  renderer.portrayalRegistry().registerAreaFillRuleForStyle(
    "area/depth",
    chart_view::runtime::portrayal::AreaFillRule{
      {160U, 90U, 40U, 255U},
      {12U, 200U, 45U, 255U},
      {230U, 230U, 217U, 255U},
      1});

  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.areasRendered == 1);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const std::array<std::uint8_t, 4> fillColor{160U, 90U, 40U, 255U};
  const std::array<std::uint8_t, 4> patternColor{12U, 200U, 45U, 255U};
  bool foundFillInterior = false;
  bool foundPatternInterior = false;
  for(int y = 220; y <= 380; ++y) {
    for(int x = 280; x <= 520; ++x) {
      const auto offset = static_cast<std::size_t>((y * 800 + x) * 4);
      if(rgba[offset + 0] == fillColor[0] && rgba[offset + 1] == fillColor[1]
         && rgba[offset + 2] == fillColor[2] && rgba[offset + 3] == fillColor[3]) {
        foundFillInterior = true;
      }
      if(rgba[offset + 0] == patternColor[0] && rgba[offset + 1] == patternColor[1]
         && rgba[offset + 2] == patternColor[2] && rgba[offset + 3] == patternColor[3]) {
        foundPatternInterior = true;
      }
      if(foundFillInterior && foundPatternInterior) {
        break;
      }
    }
    if(foundFillInterior && foundPatternInterior) {
      break;
    }
  }

  REQUIRE(foundFillInterior);
  REQUIRE(foundPatternInterior);
}

TEST_CASE("FeatureLayerRenderer executes simplified S52 buoy instructions", "[renderer][rhi][portrayal][s52][point_symbol]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature buoy;
  buoy.id = 1;
  buoy.classCode = 111;
  buoy.classAcronym = "BOYSPP";
  buoy.geometry = PointGeometry{{0.0, 51.0}};

  const auto ds = makeDataset(
    "s52_simplified_buoy",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {buoy});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::portrayal::S52DisplaySettings settings;
  settings.pointSymbolMode = chart_view::runtime::portrayal::S52PointSymbolMode::kSimplified;

  chart_view::runtime::FeatureLayerRenderer renderer(settings);
  renderer.portrayalRegistry().registerSymbolRuleForStyle(
    "point/buoy",
    chart_view::runtime::portrayal::SymbolRule{{12U, 200U, 45U, 255U}, 4});

  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.pointsRendered == 1);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const std::array<std::uint8_t, 4> symbolColor{12U, 200U, 45U, 255U};
  const std::array<std::uint8_t, 4> background{230U, 230U, 217U, 255U};
  REQUIRE(pixelMatches(rgba, 800, 400, 300, symbolColor));
  REQUIRE(pixelMatches(rgba, 800, 402, 300, symbolColor));
  REQUIRE(pixelMatches(rgba, 800, 397, 300, background));
}

TEST_CASE("FeatureLayerRenderer renders basic labels for named features", "[renderer][rhi][portrayal][label]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature namedPoint;
  namedPoint.id = 1;
  namedPoint.classCode = 700;
  namedPoint.classAcronym = "WRECKS";
  namedPoint.geometry = PointGeometry{{0.0, 51.0}};
  namedPoint.attributes["OBJNAM"] = std::string("AA");

  const auto ds = makeDataset(
    "label_render",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {namedPoint});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::FeatureLayerRenderer renderer;
  renderer.portrayalRegistry().registerTextRuleForStyle(
    "text/default",
    chart_view::runtime::portrayal::TextRule{{12U, 200U, 45U, 255U}, 12U});

  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.pointsRendered == 1);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  bool foundLabelPixel = false;
  for(int y = 288; y <= 312; ++y) {
    for(int x = 404; x <= 440; ++x) {
      const auto offset = static_cast<std::size_t>((y * 800 + x) * 4);
      if(rgba[offset + 0] == 12U && rgba[offset + 1] == 200U && rgba[offset + 2] == 45U
         && rgba[offset + 3] == 255U) {
        foundLabelPixel = true;
        break;
      }
    }
    if(foundLabelPixel) {
      break;
    }
  }

  REQUIRE(foundLabelPixel);
}

TEST_CASE("FeatureLayerRenderer skips suppressed S52 soundings", "[renderer][rhi][portrayal][s52][suppressed]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature sounding;
  sounding.id = 1;
  sounding.classCode = 129;
  sounding.classAcronym = "SOUNDG";
  sounding.geometry = PointGeometry{{0.0, 51.0}};
  sounding.attributes["VALSOU"] = 9.1;

  const auto ds = makeDataset(
    "suppressed_sounding",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {sounding});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::portrayal::S52DisplaySettings settings;
  settings.showSoundings = false;

  chart_view::runtime::FeatureLayerRenderer renderer(settings);
  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.pointsRendered == 0);
  REQUIRE(result.totalVertices == 0);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const std::array<std::uint8_t, 4> background{230U, 230U, 217U, 255U};
  REQUIRE(pixelMatches(rgba, 800, 400, 300, background));
}

TEST_CASE("FeatureLayerRenderer applies S57-specific portrayal styles for key classes", "[renderer][rhi][portrayal][s57]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature wreck;
  wreck.id = 1;
  wreck.classAcronym = "WRECKS";
  wreck.geometry = PointGeometry{{0.0, 51.0}};

  Feature fairway;
  fairway.id = 2;
  fairway.classAcronym = "FAIRWY";
  fairway.geometry = LineGeometry{{{-0.12, 50.96}, {0.12, 50.96}}};

  Feature landArea;
  landArea.id = 3;
  landArea.classAcronym = "LNDARE";
  landArea.geometry = AreaGeometry{{{-0.10, 50.88}, {0.10, 50.88}, {0.10, 51.04}, {-0.10, 51.04}}, {}};

  const auto ds = makeDataset(
    "s57_semantic_styles",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {wreck, fairway, landArea});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::FeatureLayerRenderer renderer;
  renderer.portrayalRegistry().registerSymbolRuleForStyle(
    "point/danger",
    chart_view::runtime::portrayal::SymbolRule{{210U, 92U, 28U, 255U}, 4});
  renderer.portrayalRegistry().registerLineStyleRuleForStyle(
    "line/channel",
    chart_view::runtime::portrayal::LineStyleRule{{24U, 116U, 86U, 255U}, 2});
  renderer.portrayalRegistry().registerAreaFillRuleForStyle(
    "area/land",
    chart_view::runtime::portrayal::AreaFillRule{
      {196U, 190U, 137U, 255U},
      {110U, 96U, 52U, 255U},
      {230U, 230U, 217U, 255U},
      1});

  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const auto hasColor = [&](const std::array<std::uint8_t, 4> &color) {
    return std::ranges::any_of(
      std::views::iota(std::size_t{0}, rgba.size() / 4U),
      [&](std::size_t pixelIndex) {
        const auto offset = pixelIndex * 4U;
        return rgba[offset + 0] == color[0] && rgba[offset + 1] == color[1]
            && rgba[offset + 2] == color[2] && rgba[offset + 3] == color[3];
      });
  };

  REQUIRE(hasColor({210U, 92U, 28U, 255U}));
  REQUIRE(hasColor({24U, 116U, 86U, 255U}));
  REQUIRE(hasColor({196U, 190U, 137U, 255U}));
}

TEST_CASE("FeatureLayerRenderer applies S101-specific portrayal styles for key classes", "[renderer][rhi][portrayal][s101]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature wreck;
  wreck.id = 1;
  wreck.classAcronym = "Wreck";
  wreck.geometry = PointGeometry{{0.0, 51.0}};

  Feature fairway;
  fairway.id = 2;
  fairway.classAcronym = "Fairway";
  fairway.geometry = LineGeometry{{{-0.12, 50.96}, {0.12, 50.96}}};

  Feature landArea;
  landArea.id = 3;
  landArea.classAcronym = "LandArea";
  landArea.geometry = AreaGeometry{{{-0.10, 50.88}, {0.10, 50.88}, {0.10, 51.04}, {-0.10, 51.04}}, {}};

  const auto ds = makeDataset(
    "s101_semantic_styles",
    chart_view_chart_source_s101,
    {-1.0, 50.0, 1.0, 52.0},
    {wreck, fairway, landArea});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::FeatureLayerRenderer renderer;
  renderer.portrayalRegistry().registerSymbolRuleForStyle(
    "point/danger",
    chart_view::runtime::portrayal::SymbolRule{{176U, 68U, 22U, 255U}, 4});
  renderer.portrayalRegistry().registerLineStyleRuleForStyle(
    "line/channel",
    chart_view::runtime::portrayal::LineStyleRule{{18U, 122U, 150U, 255U}, 2});
  renderer.portrayalRegistry().registerAreaFillRuleForStyle(
    "area/land",
    chart_view::runtime::portrayal::AreaFillRule{
      {188U, 178U, 112U, 255U},
      {96U, 88U, 42U, 255U},
      {230U, 230U, 217U, 255U},
      1});

  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const auto hasColor = [&](const std::array<std::uint8_t, 4> &color) {
    return std::ranges::any_of(
      std::views::iota(std::size_t{0}, rgba.size() / 4U),
      [&](std::size_t pixelIndex) {
        const auto offset = pixelIndex * 4U;
        return rgba[offset + 0] == color[0] && rgba[offset + 1] == color[1]
            && rgba[offset + 2] == color[2] && rgba[offset + 3] == color[3];
      });
  };

  REQUIRE(hasColor({176U, 68U, 22U, 255U}));
  REQUIRE(hasColor({18U, 122U, 150U, 255U}));
  REQUIRE(hasColor({188U, 178U, 112U, 255U}));
}

TEST_CASE("FeatureLayerRenderer skips non-finite projected geometry", "[renderer][rhi][guards]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature validPoint;
  validPoint.id = 1;
  validPoint.geometry = PointGeometry{{0.0, 51.0}};

  Feature invalidLine;
  invalidLine.id = 2;
  invalidLine.geometry = LineGeometry{{{0.0, 51.0}, {std::numeric_limits<double>::quiet_NaN(), 51.2}}};

  const auto ds = makeDataset(
    "guarded_render",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {validPoint, invalidLine});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::FeatureLayerRenderer renderer;
  const auto result = renderer.render(*snap, ds, backend);

  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.pointsRendered == 1);
  REQUIRE(result.linesRendered == 0);
  REQUIRE(result.totalVertices == 1);
}

TEST_CASE("FeatureLayerRenderer applies stable display priority layering", "[renderer][rhi][portrayal][priority]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature point;
  point.id = 1;
  point.geometry = PointGeometry{{0.0, 51.0}};

  Feature line;
  line.id = 2;
  line.geometry = LineGeometry{{{-0.4, 51.0}, {0.4, 51.0}}};

  Feature area;
  area.id = 3;
  area.geometry = AreaGeometry{{{-0.5, 50.6}, {0.5, 50.6}, {0.5, 51.4}, {-0.5, 51.4}}, {}};

  const auto ds = makeDataset(
    "priority_layering",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {point, line, area});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::FeatureLayerRenderer renderer;
  renderer.portrayalRegistry().registerSymbolRuleForStyle(
    "point/default",
    chart_view::runtime::portrayal::SymbolRule{{12U, 200U, 45U, 255U}, 6});
  renderer.portrayalRegistry().registerLineStyleRuleForStyle(
    "line/default",
    chart_view::runtime::portrayal::LineStyleRule{{40U, 90U, 230U, 255U}, 3});
  renderer.portrayalRegistry().registerAreaFillRuleForStyle(
    "area/default",
    chart_view::runtime::portrayal::AreaFillRule{
      {200U, 40U, 40U, 255U},
      {200U, 40U, 40U, 255U},
      {200U, 40U, 40U, 255U},
      1});

  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.pointsRendered == 1);
  REQUIRE(result.linesRendered == 1);
  REQUIRE(result.areasRendered == 1);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const auto centerX = 400;
  const auto centerY = 300;
  const auto pixelOffset = static_cast<std::size_t>((centerY * 800 + centerX) * 4);
  REQUIRE(pixelOffset + 3 < rgba.size());
  REQUIRE(rgba[pixelOffset + 0] == 12U);
  REQUIRE(rgba[pixelOffset + 1] == 200U);
  REQUIRE(rgba[pixelOffset + 2] == 45U);
  REQUIRE(rgba[pixelOffset + 3] == 255U);
}
