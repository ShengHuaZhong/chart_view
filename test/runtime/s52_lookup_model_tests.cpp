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
  REQUIRE(wreckLookup->instructions.size() == 2);
  REQUIRE(wreckLookup->instructions[0].type == S52InstructionType::kPointSymbol);
  REQUIRE(wreckLookup->instructions[0].assetId == "DANGER01");
  REQUIRE(wreckLookup->instructions[0].styleKey == "point/danger");
  REQUIRE(wreckLookup->instructions[1].type == S52InstructionType::kTextLabel);
  REQUIRE(wreckLookup->instructions[1].assetId == "TEXT01");
  REQUIRE(wreckLookup->instructions[1].styleKey == "text/default");

  const auto fairwayLookup = S52LookupModel::lookup(fairway);
  REQUIRE(fairwayLookup.has_value());
  REQUIRE(fairwayLookup->instructions.size() == 1);
  REQUIRE(fairwayLookup->instructions[0].type == S52InstructionType::kLineStyle);
  REQUIRE(fairwayLookup->instructions[0].assetId == "FAIRWY01");
  REQUIRE(fairwayLookup->instructions[0].styleKey == "line/channel");

  const auto depthAreaLookup = S52LookupModel::lookup(depthArea);
  REQUIRE(depthAreaLookup.has_value());
  REQUIRE(depthAreaLookup->instructions.size() == 1);
  REQUIRE(depthAreaLookup->instructions[0].type == S52InstructionType::kAreaPattern);
  REQUIRE(depthAreaLookup->instructions[0].assetId == "DEPARE01");
  REQUIRE(depthAreaLookup->instructions[0].styleKey == "area/depth");
}

TEST_CASE("S52LookupModel ignores unmapped classes", "[portrayal][s52][lookup]")
{
  Feature generic;
  generic.classAcronym = "ACHARE";
  generic.geometry = AreaGeometry{{{121.0, 31.0}, {121.2, 31.0}, {121.2, 31.2}}, {}};
  generic.attributes["OBJNAM"] = std::string("Anchorage");

  REQUIRE_FALSE(S52LookupModel::lookup(generic).has_value());
}
