#include <catch2/catch_test_macros.hpp>

#include "point_symbol_renderer.hpp"
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
char *AppGuard::argv[] = {const_cast<char *>("point_symbol_renderer_tests")};

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

TEST_CASE("PointSymbolRenderer draws built-in sounding glyphs", "[renderer][rhi][point_symbol]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(64, 64) == chart_view_status_ok);
  REQUIRE(backend.renderClearFrame(0.9F, 0.9F, 0.85F, 1.0F) == chart_view_status_ok);

  chart_view::runtime::PointSymbolRenderer renderer;
  const chart_view::runtime::portrayal::SymbolRule rule{{12U, 200U, 45U, 255U}, 4};

  REQUIRE(renderer.render({}, "point/sounding", {32, 32}, rule, backend));

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const std::array<std::uint8_t, 4> symbolColor{12U, 200U, 45U, 255U};
  const std::array<std::uint8_t, 4> background{230U, 230U, 217U, 255U};
  REQUIRE(pixelMatches(rgba, 64, 32, 32, symbolColor));
  REQUIRE(pixelMatches(rgba, 64, 32, 29, symbolColor));
  REQUIRE(pixelMatches(rgba, 64, 29, 32, symbolColor));
  REQUIRE(pixelMatches(rgba, 64, 34, 34, background));
}

TEST_CASE("PointSymbolRenderer exposes a narrow key-based atlas", "[renderer][rhi][point_symbol]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(64, 64) == chart_view_status_ok);

  chart_view::runtime::PointSymbolRenderer renderer;
  const chart_view::runtime::portrayal::SymbolRule rule{{196U, 46U, 46U, 255U}, 4};

  REQUIRE(renderer.render({}, "point/buoy", {20, 20}, rule, backend));
  REQUIRE(renderer.render({}, "point/beacon", {44, 20}, rule, backend));
  REQUIRE_FALSE(renderer.render({}, "point/default", {32, 44}, rule, backend));
}

TEST_CASE("PointSymbolRenderer renders compiled OpenCPN point assets using metadata-derived anchors",
          "[renderer][rhi][point_symbol][opencpn]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(64, 64) == chart_view_status_ok);
  REQUIRE(backend.renderClearFrame(0.9F, 0.9F, 0.85F, 1.0F) == chart_view_status_ok);

  chart_view::runtime::PointSymbolRenderer renderer;
  const chart_view::runtime::portrayal::SymbolRule rule{{196U, 46U, 46U, 255U}, 4};

  REQUIRE(renderer.render("ACHARE02", {}, {32, 32}, rule, backend));

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const std::array<std::uint8_t, 4> symbolColor{196U, 46U, 46U, 255U};
  const std::array<std::uint8_t, 4> background{230U, 230U, 217U, 255U};
  REQUIRE(pixelMatches(rgba, 64, 28, 28, symbolColor));
  REQUIRE(pixelMatches(rgba, 64, 29, 28, symbolColor));
  REQUIRE(pixelMatches(rgba, 64, 28, 29, symbolColor));
  REQUIRE(pixelMatches(rgba, 64, 32, 32, background));
}
