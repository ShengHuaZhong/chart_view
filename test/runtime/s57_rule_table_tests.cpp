#include <catch2/catch_test_macros.hpp>

#include "chart_data/feature.hpp"
#include "portrayal/s57_rule_table.hpp"

namespace {
using chart_view::runtime::chart_data::AreaGeometry;
using chart_view::runtime::chart_data::Feature;
using chart_view::runtime::chart_data::LineGeometry;
using chart_view::runtime::chart_data::PointGeometry;
using chart_view::runtime::portrayal::S57RuleTable;
}

TEST_CASE("S57RuleTable resolves selected key class styles", "[portrayal][s57_rule_table]")
{
  Feature wreck;
  wreck.classAcronym = "WRECKS";
  wreck.geometry = PointGeometry{{121.0, 31.0}};

  Feature landmark;
  landmark.classAcronym = "LNDMRK";
  landmark.geometry = PointGeometry{{121.1, 31.1}};

  Feature channel;
  channel.classAcronym = "FAIRWY";
  channel.geometry = LineGeometry{{{121.0, 31.0}, {121.2, 31.2}}};

  Feature landArea;
  landArea.classAcronym = "LNDARE";
  landArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.3, 31.0}, {121.3, 31.3}}, {}};

  REQUIRE(S57RuleTable::resolveStyleKey(wreck) == "point/danger");
  REQUIRE(S57RuleTable::resolveStyleKey(landmark) == "point/landmark");
  REQUIRE(S57RuleTable::resolveStyleKey(channel) == "line/channel");
  REQUIRE(S57RuleTable::resolveStyleKey(landArea) == "area/land");
}

TEST_CASE("S57RuleTable leaves unmapped classes empty", "[portrayal][s57_rule_table]")
{
  Feature generic;
  generic.classAcronym = "MORFAC";
  generic.geometry = PointGeometry{{121.0, 31.0}};

  REQUIRE(S57RuleTable::resolveStyleKey(generic).empty());
}
