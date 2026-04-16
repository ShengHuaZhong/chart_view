#include <catch2/catch_test_macros.hpp>

#include "font_fallback.hpp"
#include "glyph_cache.hpp"

#include <QGuiApplication>

namespace {

QGuiApplication &ensureApp()
{
  static int argc = 1;
  static char arg0[] = "glyph_cache_tests";
  static char *argv[] = {arg0, nullptr};
  static QGuiApplication app(argc, argv);
  return app;
}

}// namespace

TEST_CASE("GlyphCache caches rasterized fallback glyphs", "[text][font][glyph_cache]")
{
  (void)ensureApp();

  chart_view::runtime::text::GlyphCache cache;

  const auto &firstGlyph = cache.glyphFor(0x6E2FU, 18U);
  REQUIRE(firstGlyph.valid());
  REQUIRE_FALSE(firstGlyph.family.empty());
  REQUIRE_FALSE(firstGlyph.placeholder);

  const auto firstStats = cache.stats();
  REQUIRE(firstStats.misses == 1);
  REQUIRE(firstStats.hits == 0);

  const auto &secondGlyph = cache.glyphFor(0x6E2FU, 18U);
  REQUIRE(secondGlyph.valid());
  REQUIRE(secondGlyph.family == firstGlyph.family);
  REQUIRE(secondGlyph.alphaMask == firstGlyph.alphaMask);

  const auto secondStats = cache.stats();
  REQUIRE(secondStats.misses == 1);
  REQUIRE(secondStats.hits == 1);
}
