#include <catch2/catch_test_macros.hpp>

#include "chart_data/feature.hpp"
#include "portrayal/s52_conditional_symbology.hpp"
#include "portrayal/s52_display_settings.hpp"
#include "portrayal/s52_instruction_ir.hpp"
#include "portrayal/s52_lookup_model.hpp"

namespace {
using chart_view::runtime::chart_data::Feature;
using chart_view::runtime::chart_data::AreaGeometry;
using chart_view::runtime::chart_data::PointGeometry;
using chart_view::runtime::portrayal::S52ConditionalSymbology;
using chart_view::runtime::portrayal::S52ConditionalInstruction;
using chart_view::runtime::portrayal::S52ConditionalOpcode;
using chart_view::runtime::portrayal::S52DisplayCategory;
using chart_view::runtime::portrayal::S52DisplaySettings;
using chart_view::runtime::portrayal::S52InstructionType;
using chart_view::runtime::portrayal::S52LookupModel;
using chart_view::runtime::portrayal::S52LookupResult;
using chart_view::runtime::portrayal::S52PointSymbolInstruction;
using chart_view::runtime::portrayal::S52PointSymbolMode;
using chart_view::runtime::portrayal::instructionAssetId;
using chart_view::runtime::portrayal::instructionConditionalOpcode;
using chart_view::runtime::portrayal::instructionStyleKey;
using chart_view::runtime::portrayal::instructionType;

bool hasConditionalInstruction(
  const chart_view::runtime::portrayal::S52LookupResult &lookup,
  std::string_view conditionId)
{
  return std::any_of(
    lookup.instructions.begin(),
    lookup.instructions.end(),
    [&](const auto &instruction) {
      const auto *conditional =
        std::get_if<chart_view::runtime::portrayal::S52ConditionalInstruction>(&instruction);
      return conditional != nullptr && conditional->conditionId == conditionId;
    });
}

bool hasConditionalOpcode(
  const chart_view::runtime::portrayal::S52LookupResult &lookup,
  S52ConditionalOpcode opcode)
{
  return std::any_of(
    lookup.instructions.begin(),
    lookup.instructions.end(),
    [&](const auto &instruction) { return instructionConditionalOpcode(instruction) == opcode; });
}
}

TEST_CASE("S52ConditionalSymbology switches buoy assets for simplified point mode", "[portrayal][s52][conditional]")
{
  Feature buoy;
  buoy.classAcronym = "BOYSPP";
  buoy.geometry = PointGeometry{{121.0, 31.0}};

  S52DisplaySettings settings;
  settings.pointSymbolMode = S52PointSymbolMode::kSimplified;

  const auto lookup = S52ConditionalSymbology::apply(buoy, settings, S52LookupModel::lookup(buoy, settings));
  REQUIRE(lookup.has_value());
  REQUIRE_FALSE(lookup->suppressed);
  REQUIRE(instructionAssetId(lookup->instructions.front()) == "BOYSPP11");
}

TEST_CASE("S52ConditionalSymbology removes label instructions when text labels are disabled", "[portrayal][s52][conditional]")
{
  Feature wreck;
  wreck.classAcronym = "WRECKS";
  wreck.geometry = PointGeometry{{121.0, 31.0}};
  wreck.attributes["OBJNAM"] = std::string("Named Wreck");

  S52DisplaySettings settings;
  settings.showTextLabels = false;

  const auto lookup = S52ConditionalSymbology::apply(wreck, settings, S52LookupModel::lookup(wreck, settings));
  REQUIRE(lookup.has_value());
  REQUIRE_FALSE(lookup->suppressed);
  REQUIRE(lookup->instructions.size() == 1);
  REQUIRE(std::none_of(
    lookup->instructions.begin(),
    lookup->instructions.end(),
    [](const auto &instruction) { return instructionType(instruction) == S52InstructionType::kTextLabel; }));
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
    S52ConditionalSymbology::apply(sounding, settings, S52LookupModel::lookup(sounding, settings));
  REQUIRE(lookup.has_value());
  REQUIRE(lookup->suppressed);
  REQUIRE(hasConditionalInstruction(*lookup, "SOUNDG02"));
}

TEST_CASE("S52ConditionalSymbology suppresses rules beyond the active display category", "[portrayal][s52][conditional]")
{
  Feature wreck;
  wreck.classAcronym = "WRECKS";
  wreck.geometry = PointGeometry{{121.0, 31.0}};

  S52LookupResult lookup;
  lookup.lookupKey = "WRECKS";
  lookup.ruleId = "rule/wrecks/all";
  lookup.displayCategory = "all";
  lookup.instructions.push_back(S52PointSymbolInstruction{"DANGER01", "point/danger"});

  S52DisplaySettings settings;
  settings.displayCategory = S52DisplayCategory::kStandard;

  const auto conditioned = S52ConditionalSymbology::apply(wreck, settings, lookup);
  REQUIRE(conditioned.has_value());
  REQUIRE(conditioned->suppressed);
  REQUIRE(conditioned->instructions.empty());
}

TEST_CASE("S52ConditionalSymbology honors SCAMIN when the viewing scale is smaller than the feature threshold",
          "[portrayal][s52][conditional]")
{
  Feature wreck;
  wreck.classAcronym = "WRECKS";
  wreck.geometry = PointGeometry{{121.0, 31.0}};
  wreck.attributes["SCAMIN"] = 50000.0;

  S52DisplaySettings settings;
  settings.honorScamin = true;
  settings.viewingScaleDenominator = 100000.0;

  const auto lookup = S52ConditionalSymbology::apply(wreck, settings, S52LookupModel::lookup(wreck, settings));
  REQUIRE(lookup.has_value());
  REQUIRE(lookup->suppressed);
  REQUIRE(lookup->instructions.empty());

  settings.honorScamin = false;
  const auto ignored = S52ConditionalSymbology::apply(wreck, settings, S52LookupModel::lookup(wreck, settings));
  REQUIRE(ignored.has_value());
  REQUIRE_FALSE(ignored->suppressed);
  REQUIRE_FALSE(ignored->instructions.empty());
}

TEST_CASE("S52ConditionalSymbology emits chosen Phase 5 depth-area conditional outputs",
          "[portrayal][s52][conditional]")
{
  Feature depthArea;
  depthArea.classAcronym = "DEPARE";
  depthArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.1, 31.0}, {121.1, 31.1}}, {}};
  depthArea.attributes["DRVAL1"] = 1.0;
  depthArea.attributes["DRVAL2"] = 4.0;

  S52DisplaySettings settings;
  settings.twoShades = true;
  settings.shallowContourMeters = 2.0;
  settings.safetyContourMeters = 6.0;
  settings.symbolizedBoundaries = true;
  settings.shallowPattern = true;

  const auto lookup = S52ConditionalSymbology::apply(depthArea, settings, S52LookupModel::lookup(depthArea, settings));
  REQUIRE(lookup.has_value());
  REQUIRE_FALSE(lookup->suppressed);
  REQUIRE(hasConditionalInstruction(*lookup, "two_shades_depth"));
  REQUIRE(hasConditionalInstruction(*lookup, "symbolized_boundaries"));
  REQUIRE(hasConditionalInstruction(*lookup, "shallow_pattern"));
  REQUIRE(hasConditionalInstruction(*lookup, "safety_contour_alert"));

  settings.twoShades = false;
  settings.symbolizedBoundaries = false;
  settings.shallowPattern = false;
  const auto fallback = S52ConditionalSymbology::apply(depthArea, settings, S52LookupModel::lookup(depthArea, settings));
  REQUIRE(fallback.has_value());
  REQUIRE(hasConditionalInstruction(*fallback, "full_depth_shades"));
  REQUIRE(hasConditionalInstruction(*fallback, "plain_boundaries"));
  REQUIRE_FALSE(hasConditionalInstruction(*fallback, "shallow_pattern"));
}

TEST_CASE("S52ConditionalSymbology emits full-sector light conditional outputs when enabled",
          "[portrayal][s52][conditional]")
{
  Feature light;
  light.classAcronym = "LIGHTS";
  light.geometry = PointGeometry{{121.0, 31.0}};

  S52LookupResult lookup;
  lookup.lookupKey = "LIGHTS";
  lookup.ruleId = "rule/lights/point";
  lookup.displayCategory = "standard";
  lookup.instructions.push_back(S52PointSymbolInstruction{"LIGHTS01", "point/light"});

  S52DisplaySettings settings;
  settings.fullSectorLights = true;

  const auto fullSectors = S52ConditionalSymbology::apply(light, settings, lookup);
  REQUIRE(fullSectors.has_value());
  REQUIRE(hasConditionalInstruction(*fullSectors, "full_sector_lights"));

  settings.fullSectorLights = false;
  const auto baseline = S52ConditionalSymbology::apply(light, settings, lookup);
  REQUIRE(baseline.has_value());
  REQUIRE_FALSE(hasConditionalInstruction(*baseline, "full_sector_lights"));
}

TEST_CASE("S52ConditionalSymbology preserves compiled conditional opcodes and derives runtime opcode outputs",
          "[portrayal][s52][conditional][vm]")
{
  Feature anchorage;
  anchorage.classAcronym = "ACHARE";
  anchorage.geometry = AreaGeometry{{{121.0, 31.0}, {121.2, 31.0}, {121.2, 31.2}}, {}};
  anchorage.attributes["CATACH"] = std::int64_t(8);

  const auto anchorageLookup = S52ConditionalSymbology::apply(
    anchorage,
    S52DisplaySettings{},
    S52LookupModel::lookup(anchorage, S52DisplaySettings{}));
  REQUIRE(anchorageLookup.has_value());
  REQUIRE(hasConditionalInstruction(*anchorageLookup, "RESTRN01"));
  REQUIRE(hasConditionalOpcode(*anchorageLookup, S52ConditionalOpcode::kRestrn01));

  Feature light;
  light.classAcronym = "LIGHTS";
  light.geometry = PointGeometry{{121.0, 31.0}};

  S52LookupResult compiledConditionalLookup;
  compiledConditionalLookup.lookupKey = "LIGHTS";
  compiledConditionalLookup.ruleId = "compiled/lights";
  compiledConditionalLookup.displayCategory = "standard";
  compiledConditionalLookup.instructions.push_back(S52PointSymbolInstruction{"LIGHTS01", "point/landmark"});
  compiledConditionalLookup.instructions.push_back(S52ConditionalInstruction{"LIGHTS05", S52ConditionalOpcode::kLights05});

  S52DisplaySettings settings;
  settings.fullSectorLights = true;

  const auto conditioned = S52ConditionalSymbology::apply(light, settings, compiledConditionalLookup);
  REQUIRE(conditioned.has_value());
  REQUIRE(hasConditionalOpcode(*conditioned, S52ConditionalOpcode::kLights05));
  REQUIRE(hasConditionalOpcode(*conditioned, S52ConditionalOpcode::kFullSectorLights));
  REQUIRE(hasConditionalInstruction(*conditioned, "full_sector_lights"));
}
