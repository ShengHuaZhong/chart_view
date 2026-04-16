#include <catch2/catch_test_macros.hpp>

#include "line_symbol_renderer.hpp"
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
char *AppGuard::argv[] = {const_cast<char *>("line_symbol_renderer_tests")};

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

TEST_CASE("LineSymbolRenderer draws dashed depth contours", "[renderer][rhi][line_symbol]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(64, 64) == chart_view_status_ok);
  REQUIRE(backend.renderClearFrame(0.9F, 0.9F, 0.85F, 1.0F) == chart_view_status_ok);

  chart_view::runtime::LineSymbolRenderer renderer;
  const chart_view::runtime::portrayal::LineStyleRule rule{{12U, 200U, 45U, 255U}, 2};
  const std::array<chart_view::runtime::SurfacePoint, 2> points{{{8, 20}, {56, 20}}};

  REQUIRE(renderer.render("line/depth_contour", points, rule, backend));

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const std::array<std::uint8_t, 4> lineColor{12U, 200U, 45U, 255U};
  const std::array<std::uint8_t, 4> background{230U, 230U, 217U, 255U};
  REQUIRE(pixelMatches(rgba, 64, 10, 20, lineColor));
  REQUIRE(pixelMatches(rgba, 64, 18, 20, lineColor));
  REQUIRE(pixelMatches(rgba, 64, 24, 20, background));
  REQUIRE(pixelMatches(rgba, 64, 30, 20, lineColor));
}

TEST_CASE("LineSymbolRenderer exposes keyed special patterns", "[renderer][rhi][line_symbol]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(64, 64) == chart_view_status_ok);
  REQUIRE(backend.renderClearFrame(0.9F, 0.9F, 0.85F, 1.0F) == chart_view_status_ok);

  chart_view::runtime::LineSymbolRenderer renderer;
  const chart_view::runtime::portrayal::LineStyleRule rule{{196U, 46U, 46U, 255U}, 2};
  const std::array<chart_view::runtime::SurfacePoint, 2> points{{{8, 36}, {56, 36}}};

  REQUIRE(renderer.render("line/coastline", points, rule, backend));
  REQUIRE_FALSE(renderer.render("line/default", points, rule, backend));

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  const std::array<std::uint8_t, 4> lineColor{196U, 46U, 46U, 255U};
  const std::array<std::uint8_t, 4> background{230U, 230U, 217U, 255U};
  REQUIRE(pixelMatches(rgba, 64, 12, 36, lineColor));
  REQUIRE(pixelMatches(rgba, 64, 24, 36, background));
  REQUIRE(pixelMatches(rgba, 64, 27, 36, lineColor));
  REQUIRE(pixelMatches(rgba, 64, 31, 36, background));
}
