#include <catch2/catch_test_macros.hpp>

#include "catalog/chart_catalog.hpp"
#include "quilt/quilt_plan.hpp"

#include <string>
#include <utility>

namespace {

chart_view::runtime::catalog::ChartCatalogEntry makeCatalogEntry(
  std::string id,
  chart_view_chart_source_type_t sourceType,
  double nativeScale,
  std::uint32_t usageBand,
  chart_view::runtime::chart_data::Extent extent)
{
  chart_view::runtime::catalog::ChartCatalogEntry entry;
  entry.id = std::move(id);
  entry.sourceType = sourceType;
  entry.nativeScale = nativeScale;
  entry.usageBand = usageBand;
  entry.extent = extent;
  entry.sencPath = entry.id + ".senc";
  return entry;
}

}// namespace

TEST_CASE("QuiltLayer can be created from a catalog entry", "[quilt]")
{
  const auto entry = makeCatalogEntry(
    "harbor",
    chart_view_chart_source_s57,
    20000.0,
    5,
    {120.0, 30.0, 120.8, 30.8});

  const auto layer = chart_view::runtime::quilt::QuiltLayer::fromCatalogEntry(
    entry,
    2,
    {120.2, 30.2, 120.6, 30.6});

  REQUIRE(layer.chartId == "harbor");
  REQUIRE(layer.sourceType == chart_view_chart_source_s57);
  REQUIRE(layer.nativeScale == 20000.0);
  REQUIRE(layer.usageBand == 5);
  REQUIRE(layer.drawOrder == 2);
  REQUIRE(layer.fullExtent.minLon == 120.0);
  REQUIRE(layer.visibleExtent.maxLat == 30.6);
}

TEST_CASE("QuiltPlan stores viewport selection and ordered layers", "[quilt]")
{
  chart_view::runtime::quilt::QuiltPlan plan;
  REQUIRE(plan.empty());

  plan.setViewport({121.0, 31.0, 121.8, 31.8}, 90000.0);

  chart_view::runtime::quilt::QuiltSelectionResult selection;
  selection.viewportScaleDenominator = 90000.0;
  selection.candidateCount = 3;
  selection.orderedChartIds = {"approach", "harbor"};
  plan.setSelectionResult(selection);

  plan.addLayer(chart_view::runtime::quilt::QuiltLayer{
    "approach",
    "approach.senc",
    chart_view_chart_source_cm93,
    80000.0,
    4,
    0,
    {120.6, 30.4, 121.6, 31.4},
    {121.0, 31.0, 121.6, 31.4}});
  plan.addLayer(chart_view::runtime::quilt::QuiltLayer{
    "harbor",
    "harbor.senc",
    chart_view_chart_source_s57,
    20000.0,
    5,
    1,
    {121.1, 31.1, 121.5, 31.5},
    {121.1, 31.1, 121.5, 31.5}});

  REQUIRE_FALSE(plan.empty());
  REQUIRE(plan.size() == 2);
  REQUIRE(plan.viewportScaleDenominator() == 90000.0);
  REQUIRE(plan.selectionResult().candidateCount == 3);
  REQUIRE(plan.selectionResult().orderedChartIds.size() == 2);
  REQUIRE(plan.layers()[0].chartId == "approach");
  REQUIRE(plan.layers()[1].drawOrder == 1);
}

TEST_CASE("QuiltPlan clear resets accumulated state", "[quilt]")
{
  chart_view::runtime::quilt::QuiltPlan plan;
  plan.setViewport({120.0, 30.0, 121.0, 31.0}, 50000.0);
  plan.addLayer(chart_view::runtime::quilt::QuiltLayer{
    "test",
    "test.senc",
    chart_view_chart_source_s101,
    40000.0,
    5,
    0,
    {120.0, 30.0, 121.0, 31.0},
    {120.0, 30.0, 121.0, 31.0}});

  plan.clear();

  REQUIRE(plan.empty());
  REQUIRE(plan.size() == 0);
  REQUIRE(plan.viewportScaleDenominator() == 0.0);
  REQUIRE(plan.selectionResult().orderedChartIds.empty());
}
