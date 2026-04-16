#include <catch2/catch_test_macros.hpp>

#include "portrayal/s52_display_settings.hpp"

TEST_CASE("S52DisplaySettings exposes Phase 4 baseline defaults", "[portrayal][s52][settings]")
{
  chart_view::runtime::portrayal::S52DisplaySettings settings;

  REQUIRE(settings.colorScheme == chart_view::runtime::portrayal::S52ColorScheme::kDay);
  REQUIRE(settings.pointSymbolMode == chart_view::runtime::portrayal::S52PointSymbolMode::kTraditional);
  REQUIRE(settings.showSoundings);
  REQUIRE(settings.showTextLabels);
}

TEST_CASE("S52DisplaySettings supports narrow runtime-owned overrides", "[portrayal][s52][settings]")
{
  chart_view::runtime::portrayal::S52DisplaySettings settings;
  settings.colorScheme = chart_view::runtime::portrayal::S52ColorScheme::kNight;
  settings.pointSymbolMode = chart_view::runtime::portrayal::S52PointSymbolMode::kSimplified;
  settings.showSoundings = false;
  settings.showTextLabels = false;

  REQUIRE(settings.colorScheme == chart_view::runtime::portrayal::S52ColorScheme::kNight);
  REQUIRE(settings.pointSymbolMode == chart_view::runtime::portrayal::S52PointSymbolMode::kSimplified);
  REQUIRE_FALSE(settings.showSoundings);
  REQUIRE_FALSE(settings.showTextLabels);
}
