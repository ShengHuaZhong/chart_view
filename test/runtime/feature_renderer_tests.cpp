#include <catch2/catch_test_macros.hpp>

#include "feature_layer_renderer.hpp"
#include "label_layout.hpp"
#include "portrayal/s52_lookup_model.hpp"
#include "portrayal/s52_presentation_assets.hpp"
#include "projection/projection_context.hpp"
#include "projection/projected_viewport.hpp"
#include "scene_builder_from_senc.hpp"
#include "rhi_render_backend.hpp"
#include "chart_data/feature_chart_dataset.hpp"
#include "chart_data/geometry.hpp"
#include "text_label_renderer.hpp"
#include "quilt/quilt_plan.hpp"

#include <QGuiApplication>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <ranges>
#include <span>
#include <vector>

namespace {
QGuiApplication &ensureGuiApplication()
{
  if(QGuiApplication::instance() != nullptr) {
    return *static_cast<QGuiApplication *>(QGuiApplication::instance());
  }

  static QGuiApplication *app = [] {
    static int argc = 1;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
    static char arg0[] = "feature_renderer_tests";
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
    static char *argv[] = {arg0, nullptr};
    return new QGuiApplication(argc, argv);
  }();
  return *app;
}

struct AppGuard
{
  AppGuard()
  {
    (void)ensureGuiApplication();
  }
};

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

chart_view_viewport_t makeViewportDefinition(
  double centerLon,
  double centerLat,
  double scaleDenominator,
  int pixelWidth = 800,
  int pixelHeight = 600)
{
  chart_view_viewport_t vp{};
  vp.center_lon = centerLon;
  vp.center_lat = centerLat;
  vp.scale_denominator = scaleDenominator;
  vp.pixel_width = pixelWidth;
  vp.pixel_height = pixelHeight;
  return vp;
}

chart_view::runtime::ViewportState makeViewport()
{
  chart_view::runtime::ViewportState vs;
  const auto vp = makeViewportDefinition(0.0, 51.0, 100000.0, 800, 600);
  vs.set(vp);
  return vs;
}

chart_view::runtime::ViewportState makeViewport(
  double centerLon,
  double centerLat,
  double scaleDenominator,
  int pixelWidth = 800,
  int pixelHeight = 600)
{
  chart_view::runtime::ViewportState vs;
  const auto vp = makeViewportDefinition(
    centerLon,
    centerLat,
    scaleDenominator,
    pixelWidth,
    pixelHeight);
  vs.set(vp);
  return vs;
}

chart_view::runtime::SurfacePoint legacyAreaAnchor(
  const chart_view_viewport_t &viewport,
  const chart_view::runtime::chart_data::AreaGeometry &area)
{
  double lonSum = 0.0;
  double latSum = 0.0;
  for(const auto &vertex : area.exteriorRing) {
    lonSum += vertex.lon;
    latSum += vertex.lat;
  }

  const auto averageLon = lonSum / static_cast<double>(area.exteriorRing.size());
  const auto averageLat = latSum / static_cast<double>(area.exteriorRing.size());

  constexpr double kMetresPerDegLat = 111320.0;
  constexpr double kPixelsPerMetre = 3779.5275591;
  const double cosLat = std::cos(viewport.center_lat * 3.14159265358979323846 / 180.0);
  const double metresPerDegLon = kMetresPerDegLat * (cosLat > 1e-6 ? cosLat : 1e-6);

  const double halfWidthDeg =
    (viewport.pixel_width / 2.0) / kPixelsPerMetre * viewport.scale_denominator / metresPerDegLon;
  const double halfHeightDeg =
    (viewport.pixel_height / 2.0) / kPixelsPerMetre * viewport.scale_denominator / kMetresPerDegLat;

  const double scaleX = (halfWidthDeg > 1e-12) ? (1.0 / halfWidthDeg) : 1.0;
  const double scaleY = (halfHeightDeg > 1e-12) ? (1.0 / halfHeightDeg) : 1.0;
  const double ndcX = (averageLon - viewport.center_lon) * scaleX;
  const double ndcY = (averageLat - viewport.center_lat) * scaleY;

  return {
    static_cast<int>(std::lround((ndcX + 1.0) * 0.5 * static_cast<double>(viewport.pixel_width - 1))),
    static_cast<int>(std::lround((1.0 - (ndcY + 1.0) * 0.5) * static_cast<double>(viewport.pixel_height - 1)))};
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

bool frameHasColor(
  std::span<const std::uint8_t> rgba,
  const std::array<std::uint8_t, 4> &color)
{
  return std::ranges::any_of(
    std::views::iota(std::size_t{0}, rgba.size() / 4U),
    [&](std::size_t pixelIndex) {
      const auto offset = pixelIndex * 4U;
      return rgba[offset + 0] == color[0] && rgba[offset + 1] == color[1]
          && rgba[offset + 2] == color[2] && rgba[offset + 3] == color[3];
    });
}

bool regionHasColor(
  std::span<const std::uint8_t> rgba,
  int width,
  int height,
  int centerX,
  int centerY,
  int radius,
  const std::array<std::uint8_t, 4> &color)
{
  for(int y = (std::max)(0, centerY - radius); y <= (std::min)(height - 1, centerY + radius); ++y) {
    for(int x = (std::max)(0, centerX - radius); x <= (std::min)(width - 1, centerX + radius); ++x) {
      if(pixelMatches(rgba, width, x, y, color)) {
        return true;
      }
    }
  }

  return false;
}

const chart_view::runtime::portrayal::S52Instruction *findLookupInstruction(
  const std::optional<chart_view::runtime::portrayal::S52LookupResult> &lookup,
  chart_view::runtime::portrayal::S52InstructionType type)
{
  if(!lookup.has_value()) {
    return nullptr;
  }

  const auto it = std::find_if(
    lookup->instructions.begin(),
    lookup->instructions.end(),
    [type](const chart_view::runtime::portrayal::S52Instruction &instruction) {
      return chart_view::runtime::portrayal::instructionType(instruction) == type;
    });
  return it == lookup->instructions.end() ? nullptr : &(*it);
}

chart_view::runtime::SurfaceColor expectedPointAssetColor(
  const chart_view::runtime::chart_data::Feature &feature,
  const chart_view::runtime::portrayal::S52DisplaySettings &settings,
  chart_view::runtime::SurfaceColor fallback)
{
  const auto lookup = chart_view::runtime::portrayal::S52LookupModel::lookup(feature, settings);
  const auto *instruction = findLookupInstruction(
    lookup,
    chart_view::runtime::portrayal::S52InstructionType::kPointSymbol);
  if(instruction == nullptr) {
    return fallback;
  }

  chart_view::runtime::portrayal::S52PresentationAssets assets;
  if(const auto *asset = assets.findPointSymbol(chart_view::runtime::portrayal::instructionAssetId(*instruction));
     asset != nullptr) {
    return assets.resolveColor(asset->colorToken, fallback);
  }

  return fallback;
}

chart_view::runtime::SurfaceColor expectedLineAssetColor(
  const chart_view::runtime::chart_data::Feature &feature,
  const chart_view::runtime::portrayal::S52DisplaySettings &settings,
  chart_view::runtime::SurfaceColor fallback)
{
  const auto lookup = chart_view::runtime::portrayal::S52LookupModel::lookup(feature, settings);
  const auto *instruction = findLookupInstruction(
    lookup,
    chart_view::runtime::portrayal::S52InstructionType::kLineStyle);
  if(instruction == nullptr) {
    return fallback;
  }

  chart_view::runtime::portrayal::S52PresentationAssets assets;
  if(const auto *asset = assets.findLineStyle(chart_view::runtime::portrayal::instructionAssetId(*instruction));
     asset != nullptr) {
    return assets.resolveColor(asset->colorToken, fallback);
  }

  return fallback;
}

chart_view::runtime::SurfaceColor expectedAreaFillColor(
  const chart_view::runtime::chart_data::Feature &feature,
  const chart_view::runtime::portrayal::S52DisplaySettings &settings,
  chart_view::runtime::SurfaceColor fallback)
{
  auto resolvedColor = fallback;
  chart_view::runtime::portrayal::S52PresentationAssets assets;
  const auto lookup = chart_view::runtime::portrayal::S52LookupModel::lookup(feature, settings);

  if(const auto *patternInstruction = findLookupInstruction(
       lookup,
       chart_view::runtime::portrayal::S52InstructionType::kAreaPattern);
     patternInstruction != nullptr) {
    if(const auto *asset = assets.findAreaPattern(chart_view::runtime::portrayal::instructionAssetId(*patternInstruction));
       asset != nullptr) {
      resolvedColor = assets.resolveColor(asset->fillColorToken, resolvedColor);
      resolvedColor[3] = asset->fillAlpha;
    }
  }

  if(const auto *colorInstruction = findLookupInstruction(
       lookup,
       chart_view::runtime::portrayal::S52InstructionType::kAreaColor);
     colorInstruction != nullptr) {
    const auto alpha = resolvedColor[3];
    resolvedColor = assets.resolveColor(chart_view::runtime::portrayal::instructionAssetId(*colorInstruction), resolvedColor);
    resolvedColor[3] = alpha;
  }

  return resolvedColor;
}

}// namespace

TEST_CASE("FeatureLayerRenderer rejects uninitialized backend", "[renderer][rhi]")
{
  chart_view::runtime::RhiRenderBackend backend;

  auto ds = makeDataset();
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);

  chart_view::runtime::FeatureLayerRenderer renderer;
  auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_not_initialized);
}

TEST_CASE("PortrayalRegistry constructs under QGuiApplication", "[renderer][rhi][portrayal][registry]")
{
  AppGuard guard;
  chart_view::runtime::portrayal::PortrayalRegistry registry;

  const auto background = registry.canvasBackgroundColor();
  REQUIRE(background != chart_view::runtime::SurfaceColor{0U, 0U, 0U, 0U});
}

TEST_CASE("S52PresentationAssets construct under QGuiApplication", "[renderer][rhi][portrayal][assets]")
{
  AppGuard guard;
  chart_view::runtime::portrayal::S52PresentationAssets assets;

  REQUIRE(assets.findColor("NODTA") != nullptr);
}

TEST_CASE("FeatureLayerRenderer constructs with default palette-aware portrayal", "[renderer][rhi][portrayal][s52]")
{
  AppGuard guard;
  chart_view::runtime::FeatureLayerRenderer renderer;

  const auto background = renderer.portrayalRegistry().canvasBackgroundColor();
  REQUIRE(background != chart_view::runtime::SurfaceColor{0U, 0U, 0U, 0U});
  REQUIRE(renderer.s52Settings().colorScheme == chart_view::runtime::portrayal::S52ColorScheme::kDay);
}

TEST_CASE("FeatureLayerRenderer background can clear an initialized backend", "[renderer][rhi][portrayal][s52]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  chart_view::runtime::FeatureLayerRenderer renderer;
  const auto background = renderer.portrayalRegistry().canvasBackgroundColor();

  REQUIRE(
    backend.renderClearFrame(
      static_cast<float>(background[0]) / 255.0F,
      static_cast<float>(background[1]) / 255.0F,
      static_cast<float>(background[2]) / 255.0F,
      static_cast<float>(background[3]) / 255.0F)
    == chart_view_status_ok);
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
  REQUIRE(snap != nullptr);

  chart_view::runtime::FeatureLayerRenderer renderer;
  auto result = renderer.render(*snap, emptyDs, backend);
  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.totalVertices == 0);
}

TEST_CASE("FeatureLayerRenderer applies palette-sensitive canvas colors", "[renderer][rhi][portrayal][s52][palette]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend dayBackend;
  chart_view::runtime::RhiRenderBackend nightBackend;
  REQUIRE(dayBackend.initialize(800, 600) == chart_view_status_ok);
  REQUIRE(nightBackend.initialize(800, 600) == chart_view_status_ok);

  chart_view::runtime::chart_data::FeatureChartDataset emptyDs;
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.build(emptyDs, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::portrayal::S52DisplaySettings daySettings;
  daySettings.colorScheme = chart_view::runtime::portrayal::S52ColorScheme::kDay;
  chart_view::runtime::FeatureLayerRenderer dayRenderer(daySettings);

  chart_view::runtime::portrayal::S52DisplaySettings nightSettings;
  nightSettings.colorScheme = chart_view::runtime::portrayal::S52ColorScheme::kNight;
  chart_view::runtime::FeatureLayerRenderer nightRenderer(nightSettings);

  const auto dayResult = dayRenderer.render(*snap, emptyDs, dayBackend);
  const auto nightResult = nightRenderer.render(*snap, emptyDs, nightBackend);
  REQUIRE(dayResult.status == chart_view_status_ok);
  REQUIRE(nightResult.status == chart_view_status_ok);

  std::vector<std::uint8_t> dayRgba(dayBackend.frameByteSize(), 0U);
  std::vector<std::uint8_t> nightRgba(nightBackend.frameByteSize(), 0U);
  REQUIRE(dayBackend.copyFrameRgba(std::span<std::uint8_t>(dayRgba)) == chart_view_status_ok);
  REQUIRE(nightBackend.copyFrameRgba(std::span<std::uint8_t>(nightRgba)) == chart_view_status_ok);

  const auto dayBackground = dayRenderer.portrayalRegistry().canvasBackgroundColor();
  const auto nightBackground = nightRenderer.portrayalRegistry().canvasBackgroundColor();
  REQUIRE(dayBackground != nightBackground);
  REQUIRE(pixelMatches(dayRgba, 800, 0, 0, dayBackground));
  REQUIRE(pixelMatches(nightRgba, 800, 0, 0, nightBackground));
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
  const auto background = renderer.portrayalRegistry().canvasBackgroundColor();
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
  const auto background = renderer.portrayalRegistry().canvasBackgroundColor();
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

  chart_view::runtime::portrayal::S52DisplaySettings settings;
  settings.shallowPattern = true;
  settings.shallowContourMeters = 6.0;

  chart_view::runtime::FeatureLayerRenderer renderer(settings);
  renderer.portrayalRegistry().registerAreaFillRuleForStyle(
    "area/depth_shallow_pattern",
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
  REQUIRE(frameHasColor(rgba, fillColor));
  REQUIRE(frameHasColor(rgba, patternColor));
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

  const auto symbolColor = expectedPointAssetColor(ds.features().front(), settings, {12U, 200U, 45U, 255U});
  REQUIRE(regionHasColor(rgba, 800, 600, 400, 300, 6, symbolColor));
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

TEST_CASE("FeatureLayerRenderer suppresses overlapping labels in projected display space", "[renderer][rhi][portrayal][label][projected]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature shortName;
  shortName.id = 1;
  shortName.geometry = PointGeometry{{0.0, 51.0}};
  shortName.attributes["OBJNAM"] = std::string("AA");

  Feature longName;
  longName.id = 2;
  longName.geometry = PointGeometry{{0.0, 51.0}};
  longName.attributes["OBJNAM"] = std::string("BBBBBBBB");

  const auto ds = makeDataset(
    "overlapping_labels",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {shortName, longName});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::FeatureLayerRenderer renderer;
  const chart_view::runtime::portrayal::TextRule rule{{12U, 200U, 45U, 255U}, 12U};
  renderer.portrayalRegistry().registerTextRuleForStyle("text/default", rule);

  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);

  chart_view::runtime::TextLabelRenderer labelRenderer;
  const auto shortLabel = labelRenderer.layout("text/default", ds.features()[0], {400, 300}, rule);
  const auto longLabel = labelRenderer.layout("text/default", ds.features()[1], {400, 300}, rule);
  REQUIRE(shortLabel.has_value());
  REQUIRE(longLabel.has_value());
  REQUIRE(longLabel->bounds.right > shortLabel->bounds.right);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  bool foundShortLabelPixel = false;
  for(int y = (std::max)(0, shortLabel->bounds.top);
      y <= (std::min)(599, shortLabel->bounds.bottom);
      ++y) {
    for(int x = (std::max)(0, shortLabel->bounds.left);
        x <= (std::min)(799, shortLabel->bounds.right);
        ++x) {
      const auto offset = static_cast<std::size_t>((y * 800 + x) * 4);
      if(offset + 3 >= rgba.size()) {
        continue;
      }
      if(rgba[offset + 0] == 12U && rgba[offset + 1] == 200U && rgba[offset + 2] == 45U
         && rgba[offset + 3] == 255U) {
        foundShortLabelPixel = true;
        break;
      }
    }
    if(foundShortLabelPixel) {
      break;
    }
  }

  bool foundSuppressedOverlapPixel = false;
  for(int y = (std::max)(0, longLabel->bounds.top);
      y <= (std::min)(599, longLabel->bounds.bottom);
      ++y) {
    for(int x = (std::max)(0, shortLabel->bounds.right + 1);
        x <= (std::min)(799, longLabel->bounds.right);
        ++x) {
      const auto offset = static_cast<std::size_t>((y * 800 + x) * 4);
      if(offset + 3 >= rgba.size()) {
        continue;
      }
      if(rgba[offset + 0] == 12U && rgba[offset + 1] == 200U && rgba[offset + 2] == 45U
         && rgba[offset + 3] == 255U) {
        foundSuppressedOverlapPixel = true;
        break;
      }
    }
    if(foundSuppressedOverlapPixel) {
      break;
    }
  }

  REQUIRE(foundShortLabelPixel);
  REQUIRE_FALSE(foundSuppressedOverlapPixel);
}

TEST_CASE("FeatureLayerRenderer places area labels using projected anchors", "[renderer][rhi][portrayal][label][projected]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature namedArea;
  namedArea.id = 1;
  namedArea.classAcronym = "LNDARE";
  namedArea.geometry = AreaGeometry{
    {{-0.30, 67.60}, {0.20, 67.60}, {0.20, 70.80}, {-0.30, 70.80}},
    {}};
  namedArea.attributes["OBJNAM"] = std::string("Harbor");

  const auto ds = makeDataset(
    "projected_label_area",
    chart_view_chart_source_s57,
    {-1.0, 67.0, 1.0, 71.0},
    {namedArea});
  const auto vp = makeViewportDefinition(0.0, 69.4, 4000000.0, 800, 600);
  chart_view::runtime::ViewportState vs;
  vs.set(vp);

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::FeatureLayerRenderer renderer;
  const chart_view::runtime::portrayal::TextRule rule{{12U, 200U, 45U, 255U}, 12U};
  renderer.portrayalRegistry().registerTextRuleForStyle("text/default", rule);

  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);

  const auto projectionContext = chart_view::runtime::projection::ProjectionContext::createMercator();
  REQUIRE(projectionContext.isValid());

  chart_view::runtime::projection::ProjectedViewport projectedViewport;
  REQUIRE(chart_view::runtime::projection::ProjectedViewport::create(
    vp,
    projectionContext,
    projectedViewport));

  chart_view::runtime::SurfacePoint projectedAnchor{};
  REQUIRE(chart_view::runtime::label::resolveProjectedLabelAnchor(
    ds.features()[0],
    projectionContext,
    projectedViewport,
    projectedAnchor));

  const auto legacyAnchor = legacyAreaAnchor(
    vp,
    std::get<AreaGeometry>(ds.features()[0].geometry));

  chart_view::runtime::TextLabelRenderer labelRenderer;
  const auto projectedLabel = labelRenderer.layout("text/default", ds.features()[0], projectedAnchor, rule);
  const auto legacyLabel = labelRenderer.layout("text/default", ds.features()[0], legacyAnchor, rule);
  REQUIRE(projectedLabel.has_value());
  REQUIRE(legacyLabel.has_value());
  REQUIRE_FALSE(chart_view::runtime::label::labelBoundsOverlap(
    projectedLabel->bounds,
    legacyLabel->bounds,
    0));

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  bool foundProjectedLabelPixel = false;
  for(int y = (std::max)(0, projectedLabel->bounds.top);
      y <= (std::min)(599, projectedLabel->bounds.bottom);
      ++y) {
    for(int x = (std::max)(0, projectedLabel->bounds.left);
        x <= (std::min)(799, projectedLabel->bounds.right);
        ++x) {
      const auto offset = static_cast<std::size_t>((y * 800 + x) * 4);
      if(offset + 3 >= rgba.size()) {
        continue;
      }
      if(rgba[offset + 0] == 12U && rgba[offset + 1] == 200U && rgba[offset + 2] == 45U
         && rgba[offset + 3] == 255U) {
        foundProjectedLabelPixel = true;
        break;
      }
    }
    if(foundProjectedLabelPixel) {
      break;
    }
  }

  bool foundLegacyLabelPixel = false;
  for(int y = (std::max)(0, legacyLabel->bounds.top);
      y <= (std::min)(599, legacyLabel->bounds.bottom);
      ++y) {
    for(int x = (std::max)(0, legacyLabel->bounds.left);
        x <= (std::min)(799, legacyLabel->bounds.right);
        ++x) {
      const auto offset = static_cast<std::size_t>((y * 800 + x) * 4);
      if(offset + 3 >= rgba.size()) {
        continue;
      }
      if(rgba[offset + 0] == 12U && rgba[offset + 1] == 200U && rgba[offset + 2] == 45U
         && rgba[offset + 3] == 255U) {
        foundLegacyLabelPixel = true;
        break;
      }
    }
    if(foundLegacyLabelPixel) {
      break;
    }
  }

  REQUIRE(foundProjectedLabelPixel);
  REQUIRE_FALSE(foundLegacyLabelPixel);
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

  const auto background = renderer.portrayalRegistry().canvasBackgroundColor();
  REQUIRE(pixelMatches(rgba, 800, 400, 300, background));
}

TEST_CASE("FeatureLayerRenderer honors SCAMIN suppression in the active render viewport", "[renderer][rhi][portrayal][s52][scamin]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature wreck;
  wreck.id = 1;
  wreck.classCode = 159;
  wreck.classAcronym = "WRECKS";
  wreck.geometry = PointGeometry{{0.0, 51.0}};
  wreck.attributes["SCAMIN"] = 50000.0;

  const auto ds = makeDataset(
    "scamin_render",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {wreck});
  const auto vs = makeViewport(0.0, 51.0, 100000.0);

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::portrayal::S52DisplaySettings settings;
  settings.honorScamin = true;

  chart_view::runtime::FeatureLayerRenderer renderer(settings);
  renderer.portrayalRegistry().registerSymbolRuleForStyle(
    "point/danger",
    chart_view::runtime::portrayal::SymbolRule{{210U, 92U, 28U, 255U}, 4});

  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.pointsRendered == 0);
  REQUIRE(result.totalVertices == 0);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const auto background = renderer.portrayalRegistry().canvasBackgroundColor();
  REQUIRE(pixelMatches(rgba, 800, 400, 300, background));
}

TEST_CASE("FeatureLayerRenderer executes Phase 5 conditional style variants for sector lights and depth areas",
          "[renderer][rhi][portrayal][s52][conditional]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature light;
  light.id = 1;
  light.classCode = 75;
  light.classAcronym = "LIGHTS";
  light.geometry = PointGeometry{{0.0, 51.0}};

  Feature depthArea;
  depthArea.id = 2;
  depthArea.classCode = 42;
  depthArea.classAcronym = "DEPARE";
  depthArea.geometry = AreaGeometry{{{-0.08, 50.92}, {0.08, 50.92}, {0.08, 51.08}, {-0.08, 51.08}}, {}};
  depthArea.attributes["DRVAL1"] = 1.0;
  depthArea.attributes["DRVAL2"] = 4.0;

  const auto ds = makeDataset(
    "conditional_styles",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {light, depthArea});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::portrayal::S52DisplaySettings settings;
  settings.fullSectorLights = true;
  settings.shallowPattern = true;
  settings.symbolizedBoundaries = true;
  settings.safetyContourMeters = 6.0;

  chart_view::runtime::FeatureLayerRenderer renderer(settings);
  renderer.portrayalRegistry().registerSymbolRuleForStyle(
    "point/light_sector",
    chart_view::runtime::portrayal::SymbolRule{{176U, 68U, 22U, 255U}, 4});
  renderer.portrayalRegistry().registerAreaFillRuleForStyle(
    "area/depth_shallow_pattern",
    chart_view::runtime::portrayal::AreaFillRule{
      {80U, 120U, 210U, 255U},
      {20U, 170U, 90U, 255U},
      {230U, 230U, 217U, 255U},
      2});

  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.pointsRendered == 1);
  REQUIRE(result.areasRendered == 1);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const auto sectorLightColor = expectedPointAssetColor(light, settings, {176U, 68U, 22U, 255U});
  const auto shallowAreaColor = expectedAreaFillColor(depthArea, settings, {80U, 120U, 210U, 255U});
  REQUIRE(frameHasColor(rgba, sectorLightColor));
  REQUIRE(frameHasColor(rgba, shallowAreaColor));
}

TEST_CASE("FeatureLayerRenderer renders distinct two-shade and full-shade depth variants",
          "[renderer][rhi][portrayal][s52][depth_modes]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend fullBackend;
  chart_view::runtime::RhiRenderBackend twoShadeBackend;
  REQUIRE(fullBackend.initialize(800, 600) == chart_view_status_ok);
  REQUIRE(twoShadeBackend.initialize(800, 600) == chart_view_status_ok);

  Feature depthArea;
  depthArea.id = 1;
  depthArea.classCode = 42;
  depthArea.classAcronym = "DEPARE";
  depthArea.geometry = AreaGeometry{{{-0.08, 50.92}, {0.08, 50.92}, {0.08, 51.08}, {-0.08, 51.08}}, {}};
  depthArea.attributes["DRVAL1"] = 10.0;
  depthArea.attributes["DRVAL2"] = 12.0;

  const auto ds = makeDataset(
    "depth_modes",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {depthArea});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::portrayal::S52DisplaySettings fullSettings;
  fullSettings.shallowPattern = false;
  fullSettings.symbolizedBoundaries = true;
  fullSettings.safetyContourMeters = 5.0;
  fullSettings.twoShades = false;

  chart_view::runtime::portrayal::S52DisplaySettings twoShadeSettings = fullSettings;
  twoShadeSettings.twoShades = true;

  chart_view::runtime::FeatureLayerRenderer fullRenderer(fullSettings);
  fullRenderer.portrayalRegistry().registerAreaFillRuleForStyle(
    "area/depth_full_shades_symbolized_boundary",
    chart_view::runtime::portrayal::AreaFillRule{
      {24U, 140U, 210U, 255U},
      {30U, 30U, 30U, 255U},
      {230U, 230U, 217U, 255U},
      2});

  chart_view::runtime::FeatureLayerRenderer twoShadeRenderer(twoShadeSettings);
  twoShadeRenderer.portrayalRegistry().registerAreaFillRuleForStyle(
    "area/depth_two_shades_symbolized_boundary",
    chart_view::runtime::portrayal::AreaFillRule{
      {190U, 110U, 40U, 255U},
      {30U, 30U, 30U, 255U},
      {230U, 230U, 217U, 255U},
      2});

  const auto fullResult = fullRenderer.render(*snap, ds, fullBackend);
  const auto twoShadeResult = twoShadeRenderer.render(*snap, ds, twoShadeBackend);
  REQUIRE(fullResult.status == chart_view_status_ok);
  REQUIRE(twoShadeResult.status == chart_view_status_ok);
  REQUIRE(fullResult.areasRendered == 1);
  REQUIRE(twoShadeResult.areasRendered == 1);

  std::vector<std::uint8_t> fullRgba(fullBackend.frameByteSize(), 0U);
  std::vector<std::uint8_t> twoShadeRgba(twoShadeBackend.frameByteSize(), 0U);
  REQUIRE(fullBackend.copyFrameRgba(std::span<std::uint8_t>(fullRgba)) == chart_view_status_ok);
  REQUIRE(twoShadeBackend.copyFrameRgba(std::span<std::uint8_t>(twoShadeRgba)) == chart_view_status_ok);

  const auto expectedFullShadeColor = expectedAreaFillColor(depthArea, fullSettings, {24U, 140U, 210U, 255U});
  const auto expectedTwoShadeColor = expectedAreaFillColor(depthArea, twoShadeSettings, {190U, 110U, 40U, 255U});
  REQUIRE(frameHasColor(fullRgba, expectedFullShadeColor));
  REQUIRE(frameHasColor(twoShadeRgba, expectedTwoShadeColor));
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
  REQUIRE(hasColor(expectedAreaFillColor(landArea, renderer.s52Settings(), {196U, 190U, 137U, 255U})));
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

TEST_CASE("FeatureLayerRenderer prefers compiled point-asset colors over style fallback",
          "[renderer][rhi][portrayal][compiled_assets][point]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature buoy;
  buoy.id = 1;
  buoy.classAcronym = "BOYSPP";
  buoy.geometry = PointGeometry{{0.0, 51.0}};

  const auto ds = makeDataset(
    "compiled_point_asset",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {buoy});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::FeatureLayerRenderer renderer;
  const std::array<std::uint8_t, 4> fallbackColor{250U, 0U, 200U, 255U};
  renderer.portrayalRegistry().registerSymbolRuleForStyle(
    "point/buoy",
    chart_view::runtime::portrayal::SymbolRule{fallbackColor, 6});
  const auto expectedColor = expectedPointAssetColor(ds.features().front(), renderer.s52Settings(), fallbackColor);

  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.pointsRendered == 1);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);
  REQUIRE(frameHasColor(rgba, expectedColor));
}

TEST_CASE("FeatureLayerRenderer prefers compiled line-asset colors over style fallback",
          "[renderer][rhi][portrayal][compiled_assets][line]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature fairway;
  fairway.id = 1;
  fairway.classAcronym = "FAIRWY";
  fairway.geometry = LineGeometry{{{-0.2, 51.0}, {0.2, 51.0}}};

  const auto ds = makeDataset(
    "compiled_line_asset",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {fairway});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::FeatureLayerRenderer renderer;
  const std::array<std::uint8_t, 4> fallbackColor{250U, 0U, 200U, 255U};
  renderer.portrayalRegistry().registerLineStyleRuleForStyle(
    "line/channel",
    chart_view::runtime::portrayal::LineStyleRule{fallbackColor, 4});
  const auto expectedColor = expectedLineAssetColor(ds.features().front(), renderer.s52Settings(), fallbackColor);

  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.linesRendered == 1);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);
  REQUIRE(frameHasColor(rgba, expectedColor));
}

TEST_CASE("FeatureLayerRenderer applies compiled area-color tokens before style fallback",
          "[renderer][rhi][portrayal][compiled_assets][area]")
{
  using namespace chart_view::runtime::chart_data;

  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  Feature builtUpArea;
  builtUpArea.id = 1;
  builtUpArea.classAcronym = "BUAARE";
  builtUpArea.geometry = AreaGeometry{{{-0.12, 50.92}, {0.12, 50.92}, {0.12, 51.08}, {-0.12, 51.08}}, {}};
  builtUpArea.attributes["OBJNAM"] = std::string("Harbor Town");

  const auto ds = makeDataset(
    "compiled_area_color",
    chart_view_chart_source_s57,
    {-1.0, 50.0, 1.0, 52.0},
    {builtUpArea});
  auto vs = makeViewport();

  chart_view::runtime::SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);

  chart_view::runtime::FeatureLayerRenderer renderer;
  const std::array<std::uint8_t, 4> fallbackFill{250U, 0U, 200U, 255U};
  renderer.portrayalRegistry().registerAreaFillRuleForStyle(
    "area/default",
    chart_view::runtime::portrayal::AreaFillRule{
      fallbackFill,
      {40U, 40U, 40U, 255U},
      {230U, 230U, 217U, 255U},
      1});

  const auto expectedFillColor = expectedAreaFillColor(ds.features().front(), renderer.s52Settings(), fallbackFill);
  REQUIRE(expectedFillColor != fallbackFill);

  const auto result = renderer.render(*snap, ds, backend);
  REQUIRE(result.status == chart_view_status_ok);
  REQUIRE(result.areasRendered == 1);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);
  REQUIRE(frameHasColor(rgba, expectedFillColor));
}
