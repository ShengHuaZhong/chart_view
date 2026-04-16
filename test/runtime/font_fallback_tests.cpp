#include <catch2/catch_test_macros.hpp>

#include "font_fallback.hpp"

#include <QGuiApplication>

namespace {

QGuiApplication &ensureApp()
{
  static int argc = 1;
  static char arg0[] = "font_fallback_tests";
  static char *argv[] = {arg0, nullptr};
  static QGuiApplication app(argc, argv);
  return app;
}

}// namespace

TEST_CASE("FontFallbackResolver resolves CJK glyphs through runtime-owned fallback families", "[text][font][fallback]")
{
  (void)ensureApp();

  chart_view::runtime::text::FontFallbackResolver resolver;

  const auto asciiGlyph = resolver.resolve(U'A', 18U);
  REQUIRE(asciiGlyph.has_value());
  REQUIRE(asciiGlyph->valid());
  REQUIRE_FALSE(asciiGlyph->family.empty());
  REQUIRE_FALSE(asciiGlyph->fromFallback);

  const auto cjkGlyph = resolver.resolve(0x6E2FU, 18U);
  REQUIRE(cjkGlyph.has_value());
  REQUIRE(cjkGlyph->valid());
  REQUIRE_FALSE(cjkGlyph->family.empty());
}
