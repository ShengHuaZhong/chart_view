#include <catch2/catch_test_macros.hpp>

#include "feature_layer_renderer.hpp"
#include "rhi_render_backend.hpp"
#include "scene_builder_from_senc.hpp"
#include "senc/senc_reader.hpp"
#include "senc/senc_writer.hpp"
#include "portrayal/s52_display_settings.hpp"

#include <QByteArray>
#include <QGuiApplication>
#include <QtGlobal>

#include <array>
#include <ranges>
#include <span>
#include <vector>

namespace {
using chart_view::runtime::ViewportState;
using chart_view::runtime::chart_data::AreaGeometry;
using chart_view::runtime::chart_data::DatasetMeta;
using chart_view::runtime::chart_data::Extent;
using chart_view::runtime::chart_data::Feature;
using chart_view::runtime::chart_data::FeatureChartDataset;
using chart_view::runtime::chart_data::LineGeometry;
using chart_view::runtime::chart_data::PointGeometry;

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
char *AppGuard::argv[] = {const_cast<char *>("s57_symbolized_smoke_tests"), nullptr};

FeatureChartDataset makeSmokeDataset()
{
  FeatureChartDataset dataset;
  DatasetMeta meta;
  meta.name = "s57_symbolized_smoke";
  meta.sourceType = chart_view_chart_source_s57;
  meta.extent = {-1.0, 50.0, 1.0, 52.0};
  dataset.setMeta(std::move(meta));

  Feature landArea;
  landArea.id = 1;
  landArea.classCode = 71;
  landArea.classAcronym = "LNDARE";
  landArea.geometry = AreaGeometry{{{-0.40, 50.60}, {0.40, 50.60}, {0.40, 51.40}, {-0.40, 51.40}}, {}};
  dataset.addFeature(std::move(landArea));

  Feature fairway;
  fairway.id = 2;
  fairway.classCode = 81;
  fairway.classAcronym = "FAIRWY";
  fairway.geometry = LineGeometry{{{-0.35, 51.0}, {0.35, 51.0}}};
  dataset.addFeature(std::move(fairway));

  Feature buoy;
  buoy.id = 3;
  buoy.classCode = 91;
  buoy.classAcronym = "BOYSPP";
  buoy.geometry = PointGeometry{{0.0, 51.0}};
  buoy.attributes["OBJNAM"] = std::string("BY");
  dataset.addFeature(std::move(buoy));

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
  int x0,
  int y0,
  int x1,
  int y1,
  const std::array<std::uint8_t, 4> &color)
{
  for(int y = y0; y <= y1; ++y) {
    for(int x = x0; x <= x1; ++x) {
      const auto offset = static_cast<std::size_t>((y * width + x) * 4);
      if(offset + 3 >= rgba.size()) {
        continue;
      }

      if(rgba[offset + 0] == color[0] && rgba[offset + 1] == color[1]
         && rgba[offset + 2] == color[2] && rgba[offset + 3] == color[3]) {
        return true;
      }
    }
  }

  return false;
}
}// namespace

TEST_CASE("S57 symbolized smoke renders S52-backed styles, priority, and labels through SENC", "[s57][symbolized][smoke][rhi]")
{
  AppGuard guard;

  const auto sourceDataset = makeSmokeDataset();

  chart_view::runtime::senc::SencWriter writer;
  const auto sencBlob = writer.write(sourceDataset);
  REQUIRE_FALSE(sencBlob.empty());

  chart_view::runtime::senc::SencReader reader;
  const auto readback = reader.read(sencBlob);
  REQUIRE(readback.ok);
  REQUIRE(readback.dataset.featureCount() == 3);

  ViewportState viewportState;
  viewportState.set(makeViewport());

  chart_view::runtime::SceneBuilderFromSenc builder;
  const auto snapshot = builder.buildAll(readback.dataset, viewportState);
  REQUIRE(snapshot != nullptr);
  REQUIRE_FALSE(snapshot->empty());

  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(800, 600) == chart_view_status_ok);

  chart_view::runtime::portrayal::S52DisplaySettings settings;
  settings.pointSymbolMode = chart_view::runtime::portrayal::S52PointSymbolMode::kSimplified;

  chart_view::runtime::FeatureLayerRenderer renderer(settings);
  renderer.portrayalRegistry().registerAreaFillRuleForStyle(
    "area/land",
    chart_view::runtime::portrayal::AreaFillRule{
      {196U, 190U, 137U, 255U},
      {110U, 96U, 52U, 255U},
      {230U, 230U, 217U, 255U},
      1});
  renderer.portrayalRegistry().registerLineStyleRuleForStyle(
    "line/channel",
    chart_view::runtime::portrayal::LineStyleRule{{24U, 116U, 86U, 255U}, 2});
  renderer.portrayalRegistry().registerSymbolRuleForStyle(
    "point/buoy",
    chart_view::runtime::portrayal::SymbolRule{{210U, 92U, 28U, 255U}, 4});
  renderer.portrayalRegistry().registerTextRuleForStyle(
    "text/default",
    chart_view::runtime::portrayal::TextRule{{12U, 200U, 45U, 255U}, 12U});

  const auto renderResult = renderer.render(*snapshot, readback.dataset, backend);
  REQUIRE(renderResult.status == chart_view_status_ok);
  REQUIRE(renderResult.areasRendered == 1);
  REQUIRE(renderResult.linesRendered == 1);
  REQUIRE(renderResult.pointsRendered == 1);
  REQUIRE(renderResult.totalVertices == 7);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  REQUIRE(frameHasColor(rgba, {196U, 190U, 137U, 255U}));
  REQUIRE(frameHasColor(rgba, {24U, 116U, 86U, 255U}));
  REQUIRE(frameHasColor(rgba, {210U, 92U, 28U, 255U}));
  REQUIRE(regionHasColor(rgba, 800, 404, 294, 430, 310, {12U, 200U, 45U, 255U}));

  const auto centerOffset = static_cast<std::size_t>((300 * 800 + 400) * 4);
  REQUIRE(centerOffset + 3 < rgba.size());
  REQUIRE(rgba[centerOffset + 0] == 210U);
  REQUIRE(rgba[centerOffset + 1] == 92U);
  REQUIRE(rgba[centerOffset + 2] == 28U);
  REQUIRE(rgba[centerOffset + 3] == 255U);

  const auto simplifiedOffset = static_cast<std::size_t>((300 * 800 + 402) * 4);
  REQUIRE(simplifiedOffset + 3 < rgba.size());
  REQUIRE(rgba[simplifiedOffset + 0] == 210U);
  REQUIRE(rgba[simplifiedOffset + 1] == 92U);
  REQUIRE(rgba[simplifiedOffset + 2] == 28U);
  REQUIRE(rgba[simplifiedOffset + 3] == 255U);
}
