#include <catch2/catch_test_macros.hpp>

#include "chart_data/feature.hpp"
#include "portrayal/s52_lookup_model.hpp"

namespace {
using chart_view::runtime::chart_data::AreaGeometry;
using chart_view::runtime::chart_data::Feature;
using chart_view::runtime::chart_data::LineGeometry;
using chart_view::runtime::chart_data::PointGeometry;
using chart_view::runtime::portrayal::S52InstructionType;
using chart_view::runtime::portrayal::S52LookupModel;
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
  REQUIRE(wreckLookup->ruleId == "s52_point_wrecks_point_danger01_point_danger");
  REQUIRE(wreckLookup->displayCategory == "standard");
  REQUIRE(wreckLookup->displayPriority == 300);
  REQUIRE(wreckLookup->viewGroup == 33010U);
  REQUIRE(wreckLookup->instructions.size() == 2);
  REQUIRE(instructionType(wreckLookup->instructions[0]) == S52InstructionType::kPointSymbol);
  REQUIRE(instructionAssetId(wreckLookup->instructions[0]) == "DANGER01");
  REQUIRE(instructionStyleKey(wreckLookup->instructions[0]) == "point/danger");
  REQUIRE(instructionType(wreckLookup->instructions[1]) == S52InstructionType::kTextLabel);
  REQUIRE(instructionAssetId(wreckLookup->instructions[1]) == "TEXT01");
  REQUIRE(instructionStyleKey(wreckLookup->instructions[1]) == "text/default");

  const auto fairwayLookup = S52LookupModel::lookup(fairway);
  REQUIRE(fairwayLookup.has_value());
  REQUIRE(fairwayLookup->ruleId == "s52_line_fairwy_line_fairwy01_line_channel");
  REQUIRE(fairwayLookup->displayPriority == 200);
  REQUIRE(fairwayLookup->viewGroup == 23010U);
  REQUIRE(fairwayLookup->instructions.size() == 1);
  REQUIRE(instructionType(fairwayLookup->instructions[0]) == S52InstructionType::kLineStyle);
  REQUIRE(instructionAssetId(fairwayLookup->instructions[0]) == "FAIRWY01");
  REQUIRE(instructionStyleKey(fairwayLookup->instructions[0]) == "line/channel");

  const auto depthAreaLookup = S52LookupModel::lookup(depthArea);
  REQUIRE(depthAreaLookup.has_value());
  REQUIRE(depthAreaLookup->ruleId == "s52_area_depare_area_depare01_area_depth");
  REQUIRE(depthAreaLookup->displayPriority == 100);
  REQUIRE(depthAreaLookup->viewGroup == 13010U);
  REQUIRE(depthAreaLookup->instructions.size() == 1);
  REQUIRE(instructionType(depthAreaLookup->instructions[0]) == S52InstructionType::kAreaPattern);
  REQUIRE(instructionAssetId(depthAreaLookup->instructions[0]) == "DEPARE01");
  REQUIRE(instructionStyleKey(depthAreaLookup->instructions[0]) == "area/depth");
}

TEST_CASE("S52LookupModel matches richer OpenCPN-derived rows outside the old selected subset",
          "[portrayal][s52][lookup][opencpn]")
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
  REQUIRE((anchorageLookup->tableName == "Plain" || anchorageLookup->tableName == "Symbolized"));
  REQUIRE(anchorageLookup->attributeCodes == std::vector<std::string>{"CATACH8"});
  REQUIRE(anchorageLookup->instructions.size() >= 4);
  REQUIRE(instructionType(anchorageLookup->instructions[0]) == S52InstructionType::kPointSymbol);
  REQUIRE(instructionAssetId(anchorageLookup->instructions[0]) == "ACHARE02");
  REQUIRE(instructionType(anchorageLookup->instructions[1]) == S52InstructionType::kLineStyle);
  REQUIRE(instructionAssetId(anchorageLookup->instructions[1]) == "LS_DASH_2_CHMGF");
  REQUIRE(instructionType(anchorageLookup->instructions[2]) == S52InstructionType::kConditional);
  const auto *anchorageConditional =
    std::get_if<chart_view::runtime::portrayal::S52ConditionalInstruction>(&anchorageLookup->instructions[2]);
  REQUIRE(anchorageConditional != nullptr);
  REQUIRE(anchorageConditional->conditionId == "RESTRN01");
  REQUIRE(
    instructionConditionalOpcode(anchorageLookup->instructions[2])
    == chart_view::runtime::portrayal::S52ConditionalOpcode::kRestrn01);

  Feature airArea;
  airArea.classAcronym = "AIRARE";
  airArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.2, 31.0}, {121.2, 31.2}}, {}};
  const auto airAreaLookup = S52LookupModel::lookup(airArea);
  REQUIRE(airAreaLookup.has_value());
  REQUIRE((airAreaLookup->sourceRcid == "32042" || airAreaLookup->sourceRcid == "32381"));
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
