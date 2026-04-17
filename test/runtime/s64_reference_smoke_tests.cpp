#include <catch2/catch_test_macros.hpp>

#include "feature_layer_renderer.hpp"
#include "label_layout.hpp"
#include "portrayal/feature_symbolizer.hpp"
#include "projection/projection_context.hpp"
#include "projection/projected_viewport.hpp"
#include "rhi_render_backend.hpp"
#include "scene_builder_from_senc.hpp"
#include "senc/senc_reader.hpp"
#include "senc/senc_writer.hpp"
#include "text_label_renderer.hpp"

#include <QByteArray>
#include <QGuiApplication>
#include <QtGlobal>

#include <algorithm>
#include <array>
#include <optional>
#include <ranges>
#include <span>
#include <vector>

namespace {
using chart_view::runtime::ViewportState;
using chart_view::runtime::chart_data::DatasetMeta;
using chart_view::runtime::chart_data::Extent;
using chart_view::runtime::chart_data::Feature;
using chart_view::runtime::chart_data::FeatureChartDataset;
using chart_view::runtime::chart_data::PointGeometry;
using chart_view::runtime::label::LabelBounds;
using chart_view::runtime::portrayal::FeatureSymbolizer;
using chart_view::runtime::portrayal::S52DisplaySettings;
using chart_view::runtime::portrayal::S52PointSymbolMode;
using chart_view::runtime::projection::ProjectedViewport;
using chart_view::runtime::projection::ProjectionContext;

struct AppGuard
{
  static int argc;
  static char *argv[];

  AppGuard()
  {
    if(qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
      qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    }
  }

  QGuiApplication app{argc, argv};
};

int AppGuard::argc = 1;
char *AppGuard::argv[] = {const_cast<char *>("s64_reference_smoke_tests"), nullptr};

constexpr std::array<std::uint8_t, 4> kBuoyColor{210U, 92U, 28U, 255U};
constexpr std::array<std::uint8_t, 4> kSoundingColor{24U, 116U, 86U, 255U};
constexpr std::array<std::uint8_t, 4> kDangerColor{176U, 68U, 22U, 255U};
constexpr std::array<std::uint8_t, 4> kLabelColor{17U, 231U, 133U, 255U};

FeatureChartDataset makeReferenceDataset()
{
  FeatureChartDataset dataset;
  DatasetMeta meta;
  meta.name = "s64_reference_subset";
  meta.sourceType = chart_view_chart_source_s57;
  meta.extent = {-0.2, 50.9, 0.2, 51.1};
  dataset.setMeta(std::move(meta));

  Feature buoy;
  buoy.id = 1;
  buoy.classCode = 19;
  buoy.classAcronym = "BOYSPP";
  buoy.geometry = PointGeometry{{0.0, 51.0}};
  dataset.addFeature(std::move(buoy));

  Feature sounding;
  sounding.id = 2;
  sounding.classCode = 129;
  sounding.classAcronym = "SOUNDG";
  sounding.geometry = PointGeometry{{-0.05, 51.0}};
  sounding.attributes["VALSOU"] = 9.1;
  dataset.addFeature(std::move(sounding));

  Feature wreck;
  wreck.id = 3;
  wreck.classCode = 159;
  wreck.classAcronym = "WRECKS";
  wreck.geometry = PointGeometry{{0.05, 51.0}};
  wreck.attributes["OBJNAM"] = std::string("Harbor Wreck");
  wreck.attributes["NOBJNM"] = std::string(reinterpret_cast<const char *>(u8"\u6e2f\u53e3\u6c89\u8239"));
  dataset.addFeature(std::move(wreck));

  return dataset;
}

chart_view_viewport_t makeViewport() noexcept
{
  chart_view_viewport_t viewport{};
  viewport.center_lon = 0.0;
  viewport.center_lat = 51.0;
  viewport.scale_denominator = 100000.0;
  viewport.pixel_width = 800;
  viewport.pixel_height = 600;
  return viewport;
}

FeatureChartDataset roundtripDataset(const FeatureChartDataset &source)
{
  chart_view::runtime::senc::SencWriter writer;
  const auto blob = writer.write(source);
  REQUIRE_FALSE(blob.empty());

  chart_view::runtime::senc::SencReader reader;
  const auto readback = reader.read(blob);
  REQUIRE(readback.ok);
  return readback.dataset;
}

const Feature *findFeatureByClass(
  const FeatureChartDataset &dataset,
  std::string_view classAcronym)
{
  const auto it = std::find_if(
    dataset.features().begin(),
    dataset.features().end(),
    [&](const auto &feature) { return feature.classAcronym == classAcronym; });
  return it == dataset.features().end() ? nullptr : &(*it);
}

bool hasNonAsciiGlyph(const chart_view::runtime::LabelItem &label)
{
  return std::any_of(
    label.glyphText.begin(),
    label.glyphText.end(),
    [](char32_t codePoint) { return codePoint > 0x7FU; });
}

bool pixelMatches(
  std::span<const std::uint8_t> rgba,
  int width,
  int x,
  int y,
  const std::array<std::uint8_t, 4> &color)
{
  const auto offset = static_cast<std::size_t>((y * width + x) * 4);
  return offset + 3 < rgba.size()
      && rgba[offset + 0] == color[0]
      && rgba[offset + 1] == color[1]
      && rgba[offset + 2] == color[2]
      && rgba[offset + 3] == color[3];
}

bool regionHasColor(
  std::span<const std::uint8_t> rgba,
  int width,
  int height,
  const LabelBounds &bounds,
  const std::array<std::uint8_t, 4> &color)
{
  for(int y = (std::max)(0, bounds.top); y <= (std::min)(height - 1, bounds.bottom); ++y) {
    for(int x = (std::max)(0, bounds.left); x <= (std::min)(width - 1, bounds.right); ++x) {
      if(pixelMatches(rgba, width, x, y, color)) {
        return true;
      }
    }
  }

  return false;
}

std::size_t countPixelsWithColor(
  std::span<const std::uint8_t> rgba,
  const std::array<std::uint8_t, 4> &color) noexcept
{
  std::size_t matches = 0;
  for(std::size_t offset = 0; offset + 3 < rgba.size(); offset += 4) {
    if(rgba[offset + 0] == color[0] && rgba[offset + 1] == color[1] && rgba[offset + 2] == color[2]
       && rgba[offset + 3] == color[3]) {
      ++matches;
    }
  }
  return matches;
}

LabelBounds boundsAround(chart_view::runtime::SurfacePoint anchor, int radius) noexcept
{
  return {
    anchor.x - radius,
    anchor.y - radius,
    anchor.x + radius,
    anchor.y + radius};
}

struct RenderedSmoke
{
  FeatureChartDataset dataset;
  chart_view::runtime::FeatureRenderResult renderResult;
  std::vector<std::uint8_t> rgba;
  ProjectedViewport projectedViewport;
  chart_view::runtime::SurfacePoint buoyAnchor{};
  chart_view::runtime::SurfacePoint soundingAnchor{};
  chart_view::runtime::SurfacePoint wreckAnchor{};
  chart_view::runtime::LabelItem wreckLabel;
};

RenderedSmoke renderReferenceSmoke(const S52DisplaySettings &settings)
{
  const auto dataset = roundtripDataset(makeReferenceDataset());
  const auto *buoy = findFeatureByClass(dataset, "BOYSPP");
  const auto *sounding = findFeatureByClass(dataset, "SOUNDG");
  const auto *wreck = findFeatureByClass(dataset, "WRECKS");
  REQUIRE(buoy != nullptr);
  REQUIRE(sounding != nullptr);
  REQUIRE(wreck != nullptr);

  ViewportState viewportState;
  const auto viewport = makeViewport();
  viewportState.set(viewport);

  chart_view::runtime::SceneBuilderFromSenc builder;
  const auto snapshot = builder.buildAll(dataset, viewportState);
  REQUIRE(snapshot != nullptr);
  REQUIRE_FALSE(snapshot->empty());

  const auto projectionContext = ProjectionContext::createMercator();
  REQUIRE(projectionContext.isValid());

  ProjectedViewport projectedViewport;
  REQUIRE(ProjectedViewport::create(viewport, projectionContext, projectedViewport));

  chart_view::runtime::SurfacePoint buoyAnchor{};
  chart_view::runtime::SurfacePoint soundingAnchor{};
  chart_view::runtime::SurfacePoint wreckAnchor{};
  REQUIRE(chart_view::runtime::label::resolveProjectedLabelAnchor(
    *buoy,
    projectionContext,
    projectedViewport,
    buoyAnchor));
  REQUIRE(chart_view::runtime::label::resolveProjectedLabelAnchor(
    *sounding,
    projectionContext,
    projectedViewport,
    soundingAnchor));
  REQUIRE(chart_view::runtime::label::resolveProjectedLabelAnchor(
    *wreck,
    projectionContext,
    projectedViewport,
    wreckAnchor));

  chart_view::runtime::TextLabelRenderer labelRenderer;
  const chart_view::runtime::portrayal::TextRule textRule{kLabelColor, 12U};
  const auto wreckLabel = labelRenderer.layout("text/default", *wreck, wreckAnchor, textRule);
  REQUIRE(wreckLabel.has_value());

  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(viewport.pixel_width, viewport.pixel_height) == chart_view_status_ok);

  chart_view::runtime::FeatureLayerRenderer renderer(settings);
  renderer.portrayalRegistry().registerSymbolRuleForStyle(
    "point/buoy",
    chart_view::runtime::portrayal::SymbolRule{kBuoyColor, 4});
  renderer.portrayalRegistry().registerSymbolRuleForStyle(
    "point/sounding",
    chart_view::runtime::portrayal::SymbolRule{kSoundingColor, 4});
  renderer.portrayalRegistry().registerSymbolRuleForStyle(
    "point/danger",
    chart_view::runtime::portrayal::SymbolRule{kDangerColor, 4});
  renderer.portrayalRegistry().registerTextRuleForStyle("text/default", textRule);

  const auto renderResult = renderer.render(*snapshot, dataset, backend);
  REQUIRE(renderResult.status == chart_view_status_ok);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  return RenderedSmoke{
    dataset,
    renderResult,
    std::move(rgba),
    projectedViewport,
    buoyAnchor,
    soundingAnchor,
    wreckAnchor,
    std::move(*wreckLabel)};
}
}// namespace

TEST_CASE(
  "S64-inspired reference smoke keeps traditional soundings and Unicode-capable labels visible",
  "[s64][smoke][s57][reference][rhi]")
{
  AppGuard guard;

  S52DisplaySettings settings;
  const auto rendered = renderReferenceSmoke(settings);
  const auto *buoy = findFeatureByClass(rendered.dataset, "BOYSPP");
  const auto *sounding = findFeatureByClass(rendered.dataset, "SOUNDG");
  const auto *wreck = findFeatureByClass(rendered.dataset, "WRECKS");
  REQUIRE(buoy != nullptr);
  REQUIRE(sounding != nullptr);
  REQUIRE(wreck != nullptr);

  FeatureSymbolizer symbolizer(settings);
  const auto buoySymbolization = symbolizer.symbolize(*buoy);
  const auto soundingSymbolization = symbolizer.symbolize(*sounding);
  const auto wreckSymbolization = symbolizer.symbolize(*wreck);

  REQUIRE(buoySymbolization.s52Lookup.has_value());
  const auto buoyAssetId = chart_view::runtime::portrayal::instructionAssetId(
    buoySymbolization.s52Lookup->instructions.front());
  REQUIRE(buoyAssetId.starts_with("BOYSPP"));
  REQUIRE(buoyAssetId != "BOYSPP02");
  REQUIRE(soundingSymbolization.s52Lookup.has_value());
  REQUIRE_FALSE(soundingSymbolization.suppressed);
  REQUIRE(wreckSymbolization.s52Lookup.has_value());
  REQUIRE(wreckSymbolization.textKey == "text/default");

  REQUIRE(rendered.renderResult.pointsRendered == 3);
  REQUIRE(rendered.renderResult.totalVertices == 3);

  REQUIRE(regionHasColor(
    rendered.rgba,
    rendered.projectedViewport.pixelWidth,
    rendered.projectedViewport.pixelHeight,
    boundsAround(rendered.buoyAnchor, 4),
    kBuoyColor));
  REQUIRE(countPixelsWithColor(rendered.rgba, kSoundingColor) > 0);

  REQUIRE(rendered.wreckLabel.sourceAttribute == "NOBJNM");
  REQUIRE(rendered.wreckLabel.preferredNationalName);
  REQUIRE(hasNonAsciiGlyph(rendered.wreckLabel));
  REQUIRE_FALSE(rendered.wreckLabel.usedPlaceholderGlyphs);
  REQUIRE(regionHasColor(
    rendered.rgba,
    rendered.projectedViewport.pixelWidth,
    rendered.projectedViewport.pixelHeight,
    rendered.wreckLabel.bounds,
    kLabelColor));
}

TEST_CASE(
  "S64-inspired reference smoke applies simplified buoy symbols and suppresses disabled soundings and labels",
  "[s64][smoke][s57][reference][rhi]")
{
  AppGuard guard;

  S52DisplaySettings settings;
  settings.pointSymbolMode = S52PointSymbolMode::kSimplified;
  settings.showSoundings = false;
  settings.showTextLabels = false;

  const auto rendered = renderReferenceSmoke(settings);
  const auto *buoy = findFeatureByClass(rendered.dataset, "BOYSPP");
  const auto *sounding = findFeatureByClass(rendered.dataset, "SOUNDG");
  const auto *wreck = findFeatureByClass(rendered.dataset, "WRECKS");
  REQUIRE(buoy != nullptr);
  REQUIRE(sounding != nullptr);
  REQUIRE(wreck != nullptr);

  FeatureSymbolizer symbolizer(settings);
  const auto buoySymbolization = symbolizer.symbolize(*buoy);
  const auto soundingSymbolization = symbolizer.symbolize(*sounding);
  const auto wreckSymbolization = symbolizer.symbolize(*wreck);

  REQUIRE(buoySymbolization.s52Lookup.has_value());
  REQUIRE(chart_view::runtime::portrayal::instructionAssetId(
            buoySymbolization.s52Lookup->instructions.front())
          == "BOYSPP02");
  REQUIRE(soundingSymbolization.s52Lookup.has_value());
  REQUIRE(soundingSymbolization.suppressed);
  REQUIRE(wreckSymbolization.s52Lookup.has_value());
  REQUIRE(wreckSymbolization.textKey.empty());

  REQUIRE(rendered.renderResult.pointsRendered == 2);
  REQUIRE(rendered.renderResult.totalVertices == 2);

  const auto background = chart_view::runtime::portrayal::PortrayalRegistry{}.canvasBackgroundColor();
  REQUIRE(pixelMatches(
    rendered.rgba,
    rendered.projectedViewport.pixelWidth,
    rendered.buoyAnchor.x,
    rendered.buoyAnchor.y - 3,
    background));
  REQUIRE(pixelMatches(
    rendered.rgba,
    rendered.projectedViewport.pixelWidth,
    rendered.buoyAnchor.x + 2,
    rendered.buoyAnchor.y,
    kBuoyColor));
  REQUIRE(countPixelsWithColor(rendered.rgba, kSoundingColor) == 0);
  REQUIRE_FALSE(regionHasColor(
    rendered.rgba,
    rendered.projectedViewport.pixelWidth,
    rendered.projectedViewport.pixelHeight,
    rendered.wreckLabel.bounds,
    kLabelColor));
}
