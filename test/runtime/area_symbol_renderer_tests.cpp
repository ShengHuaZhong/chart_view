#include <catch2/catch_test_macros.hpp>

#include "area_symbol_renderer.hpp"
#include "rhi_render_backend.hpp"

#include <QGuiApplication>

#include <array>
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
char *AppGuard::argv[] = {const_cast<char *>("area_symbol_renderer_tests")};

bool pixelMatches(
  const std::vector<std::uint8_t> &rgba,
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

TEST_CASE("AreaSymbolRenderer overlays depth-area patterns", "[renderer][rhi][area_symbol]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(64, 64) == chart_view_status_ok);
  REQUIRE(backend.renderClearFrame(0.9F, 0.9F, 0.85F, 1.0F) == chart_view_status_ok);

  chart_view::runtime::AreaSymbolRenderer renderer;
  const chart_view::runtime::portrayal::AreaFillRule rule{
    {160U, 90U, 40U, 255U},
    {12U, 200U, 45U, 255U},
    {230U, 230U, 217U, 255U},
    1};

  const std::array<chart_view::runtime::SurfacePoint, 4> exterior{{{16, 16}, {48, 16}, {48, 48}, {16, 48}}};
  const std::vector<std::vector<chart_view::runtime::SurfacePoint>> holes;

  REQUIRE(renderer.render({}, "area/depth", exterior, holes, rule, backend));

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const std::array<std::uint8_t, 4> fillColor{160U, 90U, 40U, 255U};
  const std::array<std::uint8_t, 4> patternColor{12U, 200U, 45U, 255U};
  REQUIRE(pixelMatches(rgba, 64, 22, 25, fillColor));
  REQUIRE(pixelMatches(rgba, 64, 19, 19, patternColor));
}

TEST_CASE("AreaSymbolRenderer keeps holes clear and stays keyed by style", "[renderer][rhi][area_symbol]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(64, 64) == chart_view_status_ok);
  REQUIRE(backend.renderClearFrame(0.9F, 0.9F, 0.85F, 1.0F) == chart_view_status_ok);

  chart_view::runtime::AreaSymbolRenderer renderer;
  const chart_view::runtime::portrayal::AreaFillRule rule{
    {162U, 201U, 229U, 255U},
    {44U, 91U, 134U, 255U},
    {230U, 230U, 217U, 255U},
    1};

  const std::array<chart_view::runtime::SurfacePoint, 4> exterior{{{12, 12}, {52, 12}, {52, 52}, {12, 52}}};
  const std::vector<std::vector<chart_view::runtime::SurfacePoint>> holes{
    {{28, 28}, {36, 28}, {36, 36}, {28, 36}}};

  REQUIRE(renderer.render({}, "area/depth", exterior, holes, rule, backend));
  REQUIRE_FALSE(renderer.render({}, "area/default", exterior, holes, rule, backend));

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const std::array<std::uint8_t, 4> holeColor{230U, 230U, 217U, 255U};
  const std::array<std::uint8_t, 4> patternColor{44U, 91U, 134U, 255U};
  REQUIRE(pixelMatches(rgba, 64, 32, 32, holeColor));
  REQUIRE(pixelMatches(rgba, 64, 15, 15, patternColor));
}

TEST_CASE("AreaSymbolRenderer renders compiled OpenCPN patterns with metadata-driven spacing",
          "[renderer][rhi][area_symbol][opencpn]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(64, 64) == chart_view_status_ok);
  REQUIRE(backend.renderClearFrame(0.9F, 0.9F, 0.85F, 1.0F) == chart_view_status_ok);

  chart_view::runtime::AreaSymbolRenderer renderer;
  const chart_view::runtime::portrayal::AreaFillRule rule{
    {180U, 120U, 70U, 255U},
    {24U, 116U, 86U, 255U},
    {230U, 230U, 217U, 255U},
    1};

  const std::array<chart_view::runtime::SurfacePoint, 4> exterior{{{16, 16}, {48, 16}, {48, 48}, {16, 48}}};
  const std::vector<std::vector<chart_view::runtime::SurfacePoint>> holes;

  REQUIRE(renderer.render("PRTSUR01", {}, exterior, holes, rule, backend));

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const std::array<std::uint8_t, 4> fillColor{180U, 120U, 70U, 255U};
  const std::array<std::uint8_t, 4> patternColor{24U, 116U, 86U, 255U};
  REQUIRE(pixelMatches(rgba, 64, 22, 25, fillColor));
  REQUIRE(pixelMatches(rgba, 64, 19, 19, patternColor));
  REQUIRE(pixelMatches(rgba, 64, 25, 19, patternColor));
}
