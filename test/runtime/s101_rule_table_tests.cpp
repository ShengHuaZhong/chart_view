#include <catch2/catch_test_macros.hpp>

#include "chart_data/feature.hpp"
#include "portrayal/s101_rule_table.hpp"

namespace {
using chart_view::runtime::chart_data::AreaGeometry;
using chart_view::runtime::chart_data::Feature;
using chart_view::runtime::chart_data::LineGeometry;
using chart_view::runtime::chart_data::PointGeometry;
using chart_view::runtime::portrayal::S101RuleTable;
}

TEST_CASE("S101RuleTable resolves selected S-101 semantic classes", "[portrayal][s101]")
{
  Feature wreck;
  wreck.classAcronym = "Wreck";
  wreck.geometry = PointGeometry{{121.0, 31.0}};

  Feature landmark;
  landmark.classAcronym = "Landmark";
  landmark.geometry = PointGeometry{{121.1, 31.1}};

  Feature fairway;
  fairway.classAcronym = "Fairway";
  fairway.geometry = LineGeometry{{{121.0, 31.0}, {121.2, 31.2}}};

  Feature landArea;
  landArea.classAcronym = "LandArea";
  landArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.3, 31.0}, {121.3, 31.3}}, {}};

  REQUIRE(S101RuleTable::resolveStyleKey(wreck) == "point/danger");
  REQUIRE(S101RuleTable::resolveStyleKey(landmark) == "point/landmark");
  REQUIRE(S101RuleTable::resolveStyleKey(fairway) == "line/channel");
  REQUIRE(S101RuleTable::resolveStyleKey(landArea) == "area/land");
}

TEST_CASE("S101RuleTable supports scaffold class aliases and ignores unknown classes", "[portrayal][s101]")
{
  Feature sounding;
  sounding.classAcronym = "Sounding";
  sounding.geometry = PointGeometry{{121.0, 31.0}};

  Feature contourAlias;
  contourAlias.classAcronym = "DEPCNT";
  contourAlias.geometry = LineGeometry{{{121.0, 31.0}, {121.2, 31.2}}};

  Feature depthAreaAlias;
  depthAreaAlias.classAcronym = "DEPARE";
  depthAreaAlias.geometry = AreaGeometry{{{121.0, 31.0}, {121.3, 31.0}, {121.3, 31.3}}, {}};

  Feature generic;
  generic.classAcronym = "BridgeSpan";
  generic.geometry = LineGeometry{{{121.0, 31.0}, {121.3, 31.1}}};

  REQUIRE(S101RuleTable::resolveStyleKey(sounding) == "point/sounding");
  REQUIRE(S101RuleTable::resolveStyleKey(contourAlias) == "line/depth_contour");
  REQUIRE(S101RuleTable::resolveStyleKey(depthAreaAlias) == "area/depth");
  REQUIRE(S101RuleTable::resolveStyleKey(generic).empty());
}
