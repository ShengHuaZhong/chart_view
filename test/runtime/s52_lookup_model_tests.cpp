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

TEST_CASE("S52LookupModel ignores unmapped classes", "[portrayal][s52][lookup]")
{
  Feature generic;
  generic.classAcronym = "ACHARE";
  generic.geometry = AreaGeometry{{{121.0, 31.0}, {121.2, 31.0}, {121.2, 31.2}}, {}};
  generic.attributes["OBJNAM"] = std::string("Anchorage");

  REQUIRE_FALSE(S52LookupModel::lookup(generic).has_value());
}
