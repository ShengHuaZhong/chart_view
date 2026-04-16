#include "display_priority_model.hpp"

#include "../chart_data/feature.hpp"

namespace chart_view::runtime::portrayal {

namespace {

DisplayPriority defaultPriorityForGeometry(chart_data::GeometryType geometryType) noexcept
{
  switch(geometryType) {
  case chart_data::GeometryType::kPoint:
    return {DisplayLayerGroup::kPoints, 300U};
  case chart_data::GeometryType::kLine:
    return {DisplayLayerGroup::kLines, 200U};
  case chart_data::GeometryType::kArea:
    return {DisplayLayerGroup::kAreas, 100U};
  }

  return {};
}

}// namespace

DisplayPriority DisplayPriorityModel::resolve(
  const chart_data::Feature & /*feature*/,
  const FeatureSymbolization &symbolization) const noexcept
{
  if(symbolization.styleKey == "area/depth") {
    return {DisplayLayerGroup::kAreas, 110U};
  }
  if(symbolization.styleKey == "area/land") {
    return {DisplayLayerGroup::kAreas, 130U};
  }
  if(symbolization.styleKey == "area/restricted") {
    return {DisplayLayerGroup::kAreas, 140U};
  }
  if(symbolization.styleKey == "area/default") {
    return {DisplayLayerGroup::kAreas, 120U};
  }
  if(symbolization.styleKey == "line/default") {
    return {DisplayLayerGroup::kLines, 200U};
  }
  if(symbolization.styleKey == "line/depth_contour") {
    return {DisplayLayerGroup::kLines, 210U};
  }
  if(symbolization.styleKey == "line/coastline") {
    return {DisplayLayerGroup::kLines, 220U};
  }
  if(symbolization.styleKey == "line/channel") {
    return {DisplayLayerGroup::kLines, 230U};
  }
  if(symbolization.styleKey == "point/default") {
    return {DisplayLayerGroup::kPoints, 300U};
  }
  if(symbolization.styleKey == "point/sounding") {
    return {DisplayLayerGroup::kPoints, 310U};
  }
  if(symbolization.styleKey == "point/buoy") {
    return {DisplayLayerGroup::kPoints, 320U};
  }
  if(symbolization.styleKey == "point/beacon") {
    return {DisplayLayerGroup::kPoints, 330U};
  }
  if(symbolization.styleKey == "point/danger") {
    return {DisplayLayerGroup::kPoints, 340U};
  }
  if(symbolization.styleKey == "point/landmark") {
    return {DisplayLayerGroup::kPoints, 350U};
  }
  if(!symbolization.textKey.empty()) {
    return {DisplayLayerGroup::kText, 400U};
  }

  return defaultPriorityForGeometry(symbolization.geometryType);
}

}// namespace chart_view::runtime::portrayal
