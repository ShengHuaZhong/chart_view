#include <catch2/catch_test_macros.hpp>

#include "chart_data/feature.hpp"
#include "portrayal/s52_display_settings.hpp"
#include "portrayal/s52_lookup_model.hpp"

namespace {
using chart_view::runtime::chart_data::AreaGeometry;
using chart_view::runtime::chart_data::Feature;
using chart_view::runtime::chart_data::LineGeometry;
using chart_view::runtime::chart_data::PointGeometry;
using chart_view::runtime::portrayal::S52DisplaySettings;
using chart_view::runtime::portrayal::S52InstructionType;
using chart_view::runtime::portrayal::S52LookupModel;
using chart_view::runtime::portrayal::S52PointSymbolMode;
using chart_view::runtime::portrayal::instructionAssetId;
using chart_view::runtime::portrayal::instructionConditionalOpcode;
using chart_view::runtime::portrayal::instructionStyleKey;
using chart_view::runtime::portrayal::instructionType;
}

TEST_CASE("S52LookupModel emits baseline instructions for selected S57 classes", "[portrayal][s52][lookup]")
{
  Feature wreck;
  wreck.classAcronym = "WRECKS";
  wreck.geometry = PointGeometry{{121.0, 31.0}};
  wreck.attributes["OBJNAM"] = std::string("Named Wreck");

  Feature fairway;
  fairway.classAcronym = "FAIRWY";
  fairway.geometry = LineGeometry{{{121.0, 31.0}, {121.2, 31.2}}};

  Feature depthArea;
  depthArea.classAcronym = "DEPARE";
  depthArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.2, 31.0}, {121.2, 31.2}}, {}};

  const auto wreckLookup = S52LookupModel::lookup(wreck);
  REQUIRE(wreckLookup.has_value());
  REQUIRE(wreckLookup->lookupKey == "WRECKS");
  REQUIRE(wreckLookup->ruleId.find("paper_chart") != std::string::npos);
  REQUIRE(wreckLookup->sourceLookupId == "LU01305");
  REQUIRE(wreckLookup->sourceRcid == "LU01305");
  REQUIRE(wreckLookup->tableName == "PAPER_CHART");
  REQUIRE_FALSE(wreckLookup->displayCategory.empty());
  REQUIRE(wreckLookup->displayPriority >= 0);
  REQUIRE(wreckLookup->viewGroup > 0U);
  REQUIRE(wreckLookup->instructions.size() == 2);
  REQUIRE(std::any_of(
    wreckLookup->instructions.begin(),
    wreckLookup->instructions.end(),
    [](const auto &instruction) {
      return instructionType(instruction) == S52InstructionType::kConditional
          && instructionConditionalOpcode(instruction)
               == chart_view::runtime::portrayal::S52ConditionalOpcode::kWrecks02;
    }));
  REQUIRE(std::any_of(
    wreckLookup->instructions.begin(),
    wreckLookup->instructions.end(),
    [](const auto &instruction) {
      return instructionType(instruction) == S52InstructionType::kTextLabel
          && instructionAssetId(instruction) == "TEXT01";
    }));
  REQUIRE(wreckLookup->rawInstruction == "CS(WRECKS05)");

  const auto fairwayLookup = S52LookupModel::lookup(fairway);
  REQUIRE_FALSE(fairwayLookup.has_value());

  const auto depthAreaLookup = S52LookupModel::lookup(depthArea);
  REQUIRE(depthAreaLookup.has_value());
  REQUIRE_FALSE(depthAreaLookup->sourceRcid.empty());
  REQUIRE(depthAreaLookup->tableName == "SYMBOLIZED_BOUNDARIES");
  REQUIRE(depthAreaLookup->displayPriority >= 0);
  REQUIRE(depthAreaLookup->viewGroup > 0U);
  REQUIRE_FALSE(depthAreaLookup->instructions.empty());
  REQUIRE(std::any_of(
    depthAreaLookup->instructions.begin(),
    depthAreaLookup->instructions.end(),
    [](const auto &instruction) {
      return instructionType(instruction) == S52InstructionType::kConditional
          && (instructionConditionalOpcode(instruction)
                == chart_view::runtime::portrayal::S52ConditionalOpcode::kDepare01
              || instructionConditionalOpcode(instruction)
                   == chart_view::runtime::portrayal::S52ConditionalOpcode::kDepare02);
    }));
}

TEST_CASE("S52LookupModel matches richer official DAI rows outside the old selected subset",
          "[portrayal][s52][lookup][official]")
{
  Feature anchorage;
  anchorage.classAcronym = "ACHARE";
  anchorage.geometry = AreaGeometry{{{121.0, 31.0}, {121.2, 31.0}, {121.2, 31.2}}, {}};
  anchorage.attributes["CATACH"] = std::int64_t(8);
  anchorage.attributes["OBJNAM"] = std::string("Anchorage");

  const auto anchorageLookup = S52LookupModel::lookup(anchorage);
  REQUIRE(anchorageLookup.has_value());
  REQUIRE_FALSE(anchorageLookup->sourceRcid.empty());
  REQUIRE_FALSE(anchorageLookup->instructionFallback);
  REQUIRE(anchorageLookup->sourceLookupId == "LU00243");
  REQUIRE(anchorageLookup->sourceRcid == "LU00243");
  REQUIRE(anchorageLookup->tableName == "SYMBOLIZED_BOUNDARIES");
  REQUIRE(anchorageLookup->attributeCodes == std::vector<std::string>{"CATACH8"});
  REQUIRE(anchorageLookup->instructions.size() >= 4);
  REQUIRE(instructionType(anchorageLookup->instructions[0]) == S52InstructionType::kPointSymbol);
  REQUIRE(instructionAssetId(anchorageLookup->instructions[0]) == "ACHARE02");
  REQUIRE(instructionType(anchorageLookup->instructions[1]) == S52InstructionType::kTextLabel);
  REQUIRE(instructionType(anchorageLookup->instructions[2]) == S52InstructionType::kLineStyle);
  REQUIRE(instructionAssetId(anchorageLookup->instructions[2]) == "ACHARE51");
  REQUIRE(instructionType(anchorageLookup->instructions[3]) == S52InstructionType::kConditional);
  const auto *anchorageConditional =
    std::get_if<chart_view::runtime::portrayal::S52ConditionalInstruction>(&anchorageLookup->instructions[3]);
  REQUIRE(anchorageConditional != nullptr);
  REQUIRE(anchorageConditional->conditionId == "RESTRN01");
  REQUIRE(
    instructionConditionalOpcode(anchorageLookup->instructions[3])
    == chart_view::runtime::portrayal::S52ConditionalOpcode::kRestrn01);

  Feature airArea;
  airArea.classAcronym = "AIRARE";
  airArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.2, 31.0}, {121.2, 31.2}}, {}};
  const auto airAreaLookup = S52LookupModel::lookup(airArea);
  REQUIRE(airAreaLookup.has_value());
  REQUIRE(airAreaLookup->sourceLookupId == "LU00246");
  REQUIRE(airAreaLookup->sourceRcid == "LU00246");
  REQUIRE(airAreaLookup->tableName == "SYMBOLIZED_BOUNDARIES");
  REQUIRE_FALSE(airAreaLookup->instructionFallback);
  REQUIRE(airAreaLookup->instructions.size() == 2);
  REQUIRE(instructionType(airAreaLookup->instructions[0]) == S52InstructionType::kAreaPattern);
  REQUIRE(instructionAssetId(airAreaLookup->instructions[0]) == "AIRARE02");
  REQUIRE(instructionType(airAreaLookup->instructions[1]) == S52InstructionType::kLineStyle);
  REQUIRE(instructionAssetId(airAreaLookup->instructions[1]) == "LS_SOLD_1_LANDF");

  Feature fairway;
  fairway.classAcronym = "FAIRWY";
  fairway.geometry = AreaGeometry{{{121.0, 31.0}, {121.2, 31.0}, {121.2, 31.2}}, {}};
  fairway.attributes["ORIENT"] = std::int64_t(90);
  fairway.attributes["TRAFIC"] = std::int64_t(1);
  const auto fairwayLookup = S52LookupModel::lookup(fairway);
  REQUIRE(fairwayLookup.has_value());
  REQUIRE_FALSE(fairwayLookup->sourceRcid.empty());
  REQUIRE_FALSE(fairwayLookup->instructionFallback);
  REQUIRE_FALSE(fairwayLookup->instructions.empty());
  REQUIRE(fairwayLookup->rawInstruction.find("FAIRWY51") != std::string::npos);
}

TEST_CASE("S52LookupModel ignores unmapped classes", "[portrayal][s52][lookup]")
{
  Feature generic;
  generic.classAcronym = "ZZZZZZ";
  generic.geometry = AreaGeometry{{{121.0, 31.0}, {121.2, 31.0}, {121.2, 31.2}}, {}};

  REQUIRE_FALSE(S52LookupModel::lookup(generic).has_value());
}

TEST_CASE("S52LookupModel prefers Paper or Simplified rows according to point symbol mode",
          "[portrayal][s52][lookup][tables]")
{
  Feature buoy;
  buoy.classAcronym = "BOYSPP";
  buoy.geometry = PointGeometry{{121.0, 31.0}};

  Feature sounding;
  sounding.classAcronym = "SOUNDG";
  sounding.geometry = PointGeometry{{121.1, 31.1}};
  sounding.attributes["VALSOU"] = 9.1;

  Feature wreck;
  wreck.classAcronym = "WRECKS";
  wreck.geometry = PointGeometry{{121.2, 31.2}};

  S52DisplaySettings traditional;
  traditional.pointSymbolMode = S52PointSymbolMode::kTraditional;

  S52DisplaySettings simplified = traditional;
  simplified.pointSymbolMode = S52PointSymbolMode::kSimplified;

  const auto traditionalBuoy = S52LookupModel::lookup(buoy, traditional);
  REQUIRE(traditionalBuoy.has_value());
  REQUIRE(traditionalBuoy->sourceLookupId == "LU01075");
  REQUIRE(traditionalBuoy->sourceRcid == "LU01075");
  REQUIRE(traditionalBuoy->tableName == "PAPER_CHART");
  REQUIRE(instructionAssetId(traditionalBuoy->instructions.front()) == "BOYGEN03");

  const auto simplifiedBuoy = S52LookupModel::lookup(buoy, simplified);
  REQUIRE(simplifiedBuoy.has_value());
  REQUIRE(simplifiedBuoy->sourceLookupId == "LU00732");
  REQUIRE(simplifiedBuoy->sourceRcid == "LU00732");
  REQUIRE(simplifiedBuoy->tableName == "SIMPLIFIED");
  REQUIRE(instructionAssetId(simplifiedBuoy->instructions.front()) == "BOYSPP11");

  const auto traditionalSounding = S52LookupModel::lookup(sounding, traditional);
  REQUIRE(traditionalSounding.has_value());
  REQUIRE(traditionalSounding->sourceLookupId == "LU01284");
  REQUIRE(traditionalSounding->sourceRcid == "LU01284");
  REQUIRE(traditionalSounding->tableName == "PAPER_CHART");

  const auto traditionalWreck = S52LookupModel::lookup(wreck, traditional);
  REQUIRE(traditionalWreck.has_value());
  REQUIRE(traditionalWreck->sourceLookupId == "LU01305");
  REQUIRE(traditionalWreck->sourceRcid == "LU01305");
  REQUIRE(traditionalWreck->tableName == "PAPER_CHART");
}

TEST_CASE("S52LookupModel prefers family-specific line and area tables for Phase 6B sweeps",
          "[portrayal][s52][lookup][families]")
{
  Feature anchorage;
  anchorage.classAcronym = "ACHARE";
  anchorage.geometry = AreaGeometry{{{121.0, 31.0}, {121.2, 31.0}, {121.2, 31.2}}, {}};
  anchorage.attributes["CATACH"] = std::int64_t(8);

  S52DisplaySettings symbolizedAreaSettings;
  symbolizedAreaSettings.symbolizedBoundaries = true;

  S52DisplaySettings plainAreaSettings = symbolizedAreaSettings;
  plainAreaSettings.symbolizedBoundaries = false;

  const auto symbolizedAnchorage = S52LookupModel::lookup(anchorage, symbolizedAreaSettings);
  REQUIRE(symbolizedAnchorage.has_value());
  REQUIRE(symbolizedAnchorage->tableName == "SYMBOLIZED_BOUNDARIES");
  REQUIRE(symbolizedAnchorage->sourceLookupId == "LU00243");

  const auto plainAnchorage = S52LookupModel::lookup(anchorage, plainAreaSettings);
  REQUIRE(plainAnchorage.has_value());
  REQUIRE(plainAnchorage->tableName == "PLAIN_BOUNDARIES");
  REQUIRE(plainAnchorage->sourceLookupId == "LU00007");

  Feature pipeline;
  pipeline.classAcronym = "PIPSOL";
  pipeline.geometry = LineGeometry{{{121.0, 31.0}, {121.3, 31.3}}};

  const auto pipelineLookup = S52LookupModel::lookup(pipeline);
  REQUIRE(pipelineLookup.has_value());
  REQUIRE(pipelineLookup->tableName == "LINES");
  REQUIRE_FALSE(pipelineLookup->instructionFallback);
}
