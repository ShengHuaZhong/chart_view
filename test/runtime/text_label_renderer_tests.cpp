#include <catch2/catch_test_macros.hpp>

#include "chart_data/feature.hpp"
#include "label_layout.hpp"
#include "projection/projection_context.hpp"
#include "projection/projected_viewport.hpp"
#include "rhi_render_backend.hpp"
#include "text_label_renderer.hpp"

#include <QGuiApplication>

#include <cmath>
#include <span>
#include <vector>

namespace {

QGuiApplication &ensureApp()
{
  static int argc = 1;
  static char arg0[] = "text_label_renderer_tests";
  static char *argv[] = {arg0, nullptr};
  static QGuiApplication app(argc, argv);
  return app;
}

chart_view::runtime::RhiRenderBackend &sharedBackend()
{
  (void)ensureApp();
  static chart_view::runtime::RhiRenderBackend backend;
  return backend;
}

std::string utf8Harbor()
{
  return std::string("\xE6\xB8\xAF");
}

std::string utf8HarborA()
{
  return utf8Harbor() + "A";
}

chart_view_viewport_t makeViewport(
  double centerLon,
  double centerLat,
  double scaleDenominator,
  int pixelWidth = 800,
  int pixelHeight = 600)
{
  chart_view_viewport_t viewport{};
  viewport.center_lon = centerLon;
  viewport.center_lat = centerLat;
  viewport.scale_denominator = scaleDenominator;
  viewport.pixel_width = pixelWidth;
  viewport.pixel_height = pixelHeight;
  return viewport;
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

}// namespace

TEST_CASE("TextLabelRenderer lays out named feature labels", "[renderer][rhi][label]")
{
  (void)ensureApp();
  chart_view::runtime::TextLabelRenderer renderer;
  chart_view::runtime::chart_data::Feature feature;
  feature.attributes["OBJNAM"] = std::string("Depth Area");

  const chart_view::runtime::portrayal::TextRule rule{{12U, 200U, 45U, 255U}, 12U};
  const auto label = renderer.layout("text/default", feature, {20, 20}, rule);

  REQUIRE(label.has_value());
  REQUIRE(label->text == "Depth Area");
  REQUIRE(label->glyphText.size() == 10);
  REQUIRE(label->origin.x > 20);
  REQUIRE(label->width > 0);
  REQUIRE(label->height > 0);

  chart_view::runtime::chart_data::Feature unnamed;
  REQUIRE_FALSE(renderer.layout("text/default", unnamed, {20, 20}, rule).has_value());
}

TEST_CASE("TextLabelRenderer preserves UTF-8 labels as code-point-safe storage", "[renderer][rhi][label][unicode]")
{
  (void)ensureApp();
  chart_view::runtime::TextLabelRenderer renderer;
  chart_view::runtime::chart_data::Feature feature;
  feature.attributes["OBJNAM"] = utf8HarborA();

  const chart_view::runtime::portrayal::TextRule rule{{12U, 200U, 45U, 255U}, 12U};
  const auto label = renderer.layout("text/default", feature, {20, 20}, rule);

  REQUIRE(label.has_value());
  REQUIRE(label->text == utf8HarborA());
  REQUIRE(label->glyphText.size() == 2);
  REQUIRE(label->glyphText[0] == 0x6E2FU);
  REQUIRE(label->glyphText[1] == U'A');
  REQUIRE(label->width > 0);
  REQUIRE_FALSE(label->usedPlaceholderGlyphs);
}

TEST_CASE("TextLabelRenderer prefers national names for multilingual labels", "[renderer][rhi][label][unicode][multilingual]")
{
  (void)ensureApp();
  chart_view::runtime::TextLabelRenderer renderer;
  chart_view::runtime::chart_data::Feature feature;
  feature.attributes["OBJNAM"] = std::string("Harbor");
  feature.attributes["NOBJNM"] = utf8HarborA();

  const chart_view::runtime::portrayal::TextRule rule{{12U, 200U, 45U, 255U}, 12U};
  const auto label = renderer.layout("text/default", feature, {20, 20}, rule);

  REQUIRE(label.has_value());
  REQUIRE(label->text == utf8HarborA());
  REQUIRE(label->sourceAttribute == "NOBJNM");
  REQUIRE(label->preferredNationalName);
  REQUIRE(label->glyphText.size() == 2);
}

TEST_CASE(
  "TextLabelRenderer honors compiled preferred attribute keys before multilingual fallback",
  "[renderer][rhi][label][unicode][multilingual]")
{
  (void)ensureApp();
  chart_view::runtime::TextLabelRenderer renderer;
  chart_view::runtime::chart_data::Feature feature;
  feature.attributes["OBJNAM"] = std::string("Harbor");
  feature.attributes["NOBJNM"] = utf8HarborA();

  const chart_view::runtime::portrayal::TextRule rule{{12U, 200U, 45U, 255U}, 12U};
  const auto label = renderer.layout("text/default", feature, {20, 20}, rule, "OBJNAM");

  REQUIRE(label.has_value());
  REQUIRE(label->text == "Harbor");
  REQUIRE(label->sourceAttribute == "OBJNAM");
  REQUIRE_FALSE(label->preferredNationalName);
}

TEST_CASE("TextLabelRenderer falls back from invalid national names to object names", "[renderer][rhi][label][unicode][multilingual]")
{
  (void)ensureApp();
  chart_view::runtime::TextLabelRenderer renderer;
  chart_view::runtime::chart_data::Feature feature;
  feature.attributes["OBJNAM"] = std::string("Harbor");
  feature.attributes["NOBJNM"] = std::string("\xE6");

  const chart_view::runtime::portrayal::TextRule rule{{12U, 200U, 45U, 255U}, 12U};
  const auto label = renderer.layout("text/default", feature, {20, 20}, rule);

  REQUIRE(label.has_value());
  REQUIRE(label->text == "Harbor");
  REQUIRE(label->sourceAttribute == "OBJNAM");
  REQUIRE_FALSE(label->preferredNationalName);
}

TEST_CASE(
  "TextLabelRenderer falls back from invalid preferred national names to object names",
  "[renderer][rhi][label][unicode][multilingual]")
{
  (void)ensureApp();
  chart_view::runtime::TextLabelRenderer renderer;
  chart_view::runtime::chart_data::Feature feature;
  feature.attributes["OBJNAM"] = std::string("Harbor");
  feature.attributes["NOBJNM"] = std::string("\xE6");

  const chart_view::runtime::portrayal::TextRule rule{{12U, 200U, 45U, 255U}, 12U};
  const auto label = renderer.layout("text/default", feature, {20, 20}, rule, "NOBJNM");

  REQUIRE(label.has_value());
  REQUIRE(label->text == "Harbor");
  REQUIRE(label->sourceAttribute == "OBJNAM");
  REQUIRE_FALSE(label->preferredNationalName);
}

TEST_CASE("Projected label anchors differ from legacy geographic averages for high-latitude areas", "[renderer][rhi][label][projected]")
{
  const auto viewport = makeViewport(0.0, 69.8, 4000000.0);

  chart_view::runtime::chart_data::Feature areaFeature;
  areaFeature.geometry = chart_view::runtime::chart_data::AreaGeometry{
    {{-0.30, 68.60}, {0.20, 68.60}, {0.20, 70.80}, {-0.30, 70.80}},
    {}};

  const auto projectionContext = chart_view::runtime::projection::ProjectionContext::createMercator();
  REQUIRE(projectionContext.isValid());

  chart_view::runtime::projection::ProjectedViewport projectedViewport;
  REQUIRE(chart_view::runtime::projection::ProjectedViewport::create(
    viewport,
    projectionContext,
    projectedViewport));

  chart_view::runtime::SurfacePoint projectedAnchor{};
  REQUIRE(chart_view::runtime::label::resolveProjectedLabelAnchor(
    areaFeature,
    projectionContext,
    projectedViewport,
    projectedAnchor));

  const auto legacyAnchor = legacyAreaAnchor(
    viewport,
    std::get<chart_view::runtime::chart_data::AreaGeometry>(areaFeature.geometry));
  REQUIRE(std::abs(projectedAnchor.y - legacyAnchor.y) >= 8);
}

TEST_CASE("TextLabelRenderer draws bitmap labels", "[renderer][rhi][label]")
{
  auto &backend = sharedBackend();
  REQUIRE((backend.isInitialized() ? backend.ensureSurfaceSize(64, 64) : backend.initialize(64, 64))
          == chart_view_status_ok);
  REQUIRE(backend.renderClearFrame(0.9F, 0.9F, 0.85F, 1.0F) == chart_view_status_ok);

  chart_view::runtime::TextLabelRenderer renderer;
  chart_view::runtime::chart_data::Feature feature;
  feature.attributes["OBJNAM"] = std::string("AA");

  const chart_view::runtime::portrayal::TextRule rule{{12U, 200U, 45U, 255U}, 12U};
  const auto label = renderer.layout("text/default", feature, {20, 20}, rule);
  REQUIRE(label.has_value());

  renderer.render(*label, backend);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  bool foundLabelPixel = false;
  for(int y = label->origin.y; y < label->origin.y + label->height; ++y) {
    for(int x = label->origin.x; x < label->origin.x + label->width; ++x) {
      const auto offset = static_cast<std::size_t>((y * 64 + x) * 4);
      if(offset + 3 >= rgba.size()) {
        continue;
      }
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

TEST_CASE("TextLabelRenderer renders non-ASCII labels through runtime font fallback", "[renderer][rhi][label][unicode]")
{
  auto &backend = sharedBackend();
  REQUIRE((backend.isInitialized() ? backend.ensureSurfaceSize(64, 64) : backend.initialize(64, 64))
          == chart_view_status_ok);
  REQUIRE(backend.renderClearFrame(0.9F, 0.9F, 0.85F, 1.0F) == chart_view_status_ok);

  chart_view::runtime::TextLabelRenderer renderer;
  chart_view::runtime::chart_data::Feature feature;
  feature.attributes["OBJNAM"] = utf8Harbor();

  const chart_view::runtime::portrayal::TextRule rule{{12U, 200U, 45U, 255U}, 12U};
  const auto label = renderer.layout("text/default", feature, {20, 20}, rule);
  REQUIRE(label.has_value());
  REQUIRE(label->glyphText.size() == 1);
  REQUIRE_FALSE(label->usedPlaceholderGlyphs);

  renderer.render(*label, backend);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  bool foundLabelPixel = false;
  for(int y = label->origin.y; y < label->origin.y + label->height; ++y) {
    for(int x = label->origin.x; x < label->origin.x + label->width; ++x) {
      const auto offset = static_cast<std::size_t>((y * 64 + x) * 4);
      if(offset + 3 >= rgba.size()) {
        continue;
      }
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
