#include <catch2/catch_test_macros.hpp>

#include "chart_data/feature.hpp"
#include "portrayal/display_priority_model.hpp"
#include "portrayal/feature_symbolizer.hpp"

namespace {
using chart_view::runtime::chart_data::AreaGeometry;
using chart_view::runtime::chart_data::Feature;
using chart_view::runtime::chart_data::LineGeometry;
using chart_view::runtime::chart_data::PointGeometry;
using chart_view::runtime::portrayal::DisplayLayerGroup;
using chart_view::runtime::portrayal::DisplayPriorityModel;
using chart_view::runtime::portrayal::FeatureSymbolizer;
}

TEST_CASE("DisplayPriorityModel groups symbolized features into stable layers", "[portrayal][priority]")
{
  FeatureSymbolizer symbolizer;
  DisplayPriorityModel displayPriority;

  Feature area;
  area.geometry = AreaGeometry{{{121.0, 31.0}, {121.3, 31.0}, {121.3, 31.3}}, {}};

  Feature line;
  line.geometry = LineGeometry{{{121.0, 31.0}, {121.3, 31.2}}};

  Feature point;
  point.geometry = PointGeometry{{121.1, 31.1}};

  const auto areaPriority = displayPriority.resolve(area, symbolizer.symbolize(area));
  const auto linePriority = displayPriority.resolve(line, symbolizer.symbolize(line));
  const auto pointPriority = displayPriority.resolve(point, symbolizer.symbolize(point));

  REQUIRE(areaPriority.layerGroup == DisplayLayerGroup::kAreas);
  REQUIRE(linePriority.layerGroup == DisplayLayerGroup::kLines);
  REQUIRE(pointPriority.layerGroup == DisplayLayerGroup::kPoints);
  REQUIRE(areaPriority.priority < linePriority.priority);
  REQUIRE(linePriority.priority < pointPriority.priority);
}

TEST_CASE("DisplayPriorityModel keeps specialized styles ordered within a layer", "[portrayal][priority]")
{
  FeatureSymbolizer symbolizer;
  DisplayPriorityModel displayPriority;

  Feature genericLine;
  genericLine.geometry = LineGeometry{{{121.0, 31.0}, {121.3, 31.2}}};

  Feature depthContour;
  depthContour.geometry = LineGeometry{{{121.0, 31.0}, {121.4, 31.3}}};
  depthContour.attributes["VALDCO"] = 20.0;

  Feature sounding;
  sounding.geometry = PointGeometry{{121.1, 31.1}};
  sounding.attributes["VALSOU"] = 8.0;

  const auto genericLinePriority =
    displayPriority.resolve(genericLine, symbolizer.symbolize(genericLine));
  const auto contourPriority =
    displayPriority.resolve(depthContour, symbolizer.symbolize(depthContour));
  const auto soundingPriority =
    displayPriority.resolve(sounding, symbolizer.symbolize(sounding));

  REQUIRE(genericLinePriority.layerGroup == DisplayLayerGroup::kLines);
  REQUIRE(contourPriority.layerGroup == DisplayLayerGroup::kLines);
  REQUIRE(contourPriority.priority > genericLinePriority.priority);
  REQUIRE(soundingPriority.layerGroup == DisplayLayerGroup::kPoints);
  REQUIRE(soundingPriority.priority > contourPriority.priority);
}

TEST_CASE("DisplayPriorityModel keeps S57 semantic styles on their geometry layers", "[portrayal][priority][s57]")
{
  FeatureSymbolizer symbolizer;
  DisplayPriorityModel displayPriority;

  Feature dangerPoint;
  dangerPoint.classAcronym = "WRECKS";
  dangerPoint.geometry = PointGeometry{{121.1, 31.1}};
  dangerPoint.attributes["OBJNAM"] = std::string("Wreck");

  Feature channelLine;
  channelLine.classAcronym = "FAIRWY";
  channelLine.geometry = LineGeometry{{{121.0, 31.0}, {121.3, 31.2}}};

  Feature landArea;
  landArea.classAcronym = "LNDARE";
  landArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.3, 31.0}, {121.3, 31.3}}, {}};

  const auto pointPriority = displayPriority.resolve(dangerPoint, symbolizer.symbolize(dangerPoint));
  const auto linePriority = displayPriority.resolve(channelLine, symbolizer.symbolize(channelLine));
  const auto areaPriority = displayPriority.resolve(landArea, symbolizer.symbolize(landArea));

  REQUIRE(pointPriority.layerGroup == DisplayLayerGroup::kPoints);
  REQUIRE(linePriority.layerGroup == DisplayLayerGroup::kLines);
  REQUIRE(areaPriority.layerGroup == DisplayLayerGroup::kAreas);
  REQUIRE(areaPriority.priority < linePriority.priority);
  REQUIRE(linePriority.priority < pointPriority.priority);
}
