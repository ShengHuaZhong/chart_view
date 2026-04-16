#include <catch2/catch_test_macros.hpp>

#include "chart_data/feature.hpp"
#include "portrayal/s52_conditional_symbology.hpp"
#include "portrayal/s52_display_settings.hpp"
#include "portrayal/s52_lookup_model.hpp"

namespace {
using chart_view::runtime::chart_data::Feature;
using chart_view::runtime::chart_data::PointGeometry;
using chart_view::runtime::portrayal::S52ConditionalSymbology;
using chart_view::runtime::portrayal::S52DisplaySettings;
using chart_view::runtime::portrayal::S52LookupModel;
using chart_view::runtime::portrayal::S52PointSymbolMode;
using chart_view::runtime::portrayal::instructionAssetId;
}

TEST_CASE("S52ConditionalSymbology switches buoy assets for simplified point mode", "[portrayal][s52][conditional]")
{
  Feature buoy;
  buoy.classAcronym = "BOYSPP";
  buoy.geometry = PointGeometry{{121.0, 31.0}};

  S52DisplaySettings settings;
  settings.pointSymbolMode = S52PointSymbolMode::kSimplified;

  const auto lookup = S52ConditionalSymbology::apply(buoy, settings, S52LookupModel::lookup(buoy));
  REQUIRE(lookup.has_value());
  REQUIRE_FALSE(lookup->suppressed);
  REQUIRE(instructionAssetId(lookup->instructions.front()) == "BOYSPP02");
}

TEST_CASE("S52ConditionalSymbology removes label instructions when text labels are disabled", "[portrayal][s52][conditional]")
{
  Feature wreck;
  wreck.classAcronym = "WRECKS";
  wreck.geometry = PointGeometry{{121.0, 31.0}};
  wreck.attributes["OBJNAM"] = std::string("Named Wreck");

  S52DisplaySettings settings;
  settings.showTextLabels = false;

  const auto lookup = S52ConditionalSymbology::apply(wreck, settings, S52LookupModel::lookup(wreck));
  REQUIRE(lookup.has_value());
  REQUIRE(lookup->instructions.size() == 1);
  REQUIRE(instructionAssetId(lookup->instructions.front()) == "DANGER01");
}

TEST_CASE("S52ConditionalSymbology suppresses soundings when disabled", "[portrayal][s52][conditional]")
{
  Feature sounding;
  sounding.classAcronym = "SOUNDG";
  sounding.geometry = PointGeometry{{121.0, 31.0}};
  sounding.attributes["VALSOU"] = 12.5;

  S52DisplaySettings settings;
  settings.showSoundings = false;

  const auto lookup =
    S52ConditionalSymbology::apply(sounding, settings, S52LookupModel::lookup(sounding));
  REQUIRE(lookup.has_value());
  REQUIRE(lookup->suppressed);
  REQUIRE(lookup->instructions.empty());
}
