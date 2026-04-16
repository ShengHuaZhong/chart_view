#include <catch2/catch_test_macros.hpp>

#include "chart_data/feature.hpp"
#include "rhi_render_backend.hpp"
#include "text_label_renderer.hpp"

#include <QGuiApplication>

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
char *AppGuard::argv[] = {const_cast<char *>("text_label_renderer_tests")};

}// namespace

TEST_CASE("TextLabelRenderer lays out named feature labels", "[renderer][rhi][label]")
{
  chart_view::runtime::TextLabelRenderer renderer;
  chart_view::runtime::chart_data::Feature feature;
  feature.attributes["OBJNAM"] = std::string("Depth Area");

  const chart_view::runtime::portrayal::TextRule rule{{12U, 200U, 45U, 255U}, 12U};
  const auto label = renderer.layout("text/default", feature, {20, 20}, rule);

  REQUIRE(label.has_value());
  REQUIRE(label->text == "Depth Area");
  REQUIRE(label->origin.x > 20);
  REQUIRE(label->width > 0);
  REQUIRE(label->height > 0);

  chart_view::runtime::chart_data::Feature unnamed;
  REQUIRE_FALSE(renderer.layout("text/default", unnamed, {20, 20}, rule).has_value());
}

TEST_CASE("TextLabelRenderer draws bitmap labels", "[renderer][rhi][label]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(64, 64) == chart_view_status_ok);
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
