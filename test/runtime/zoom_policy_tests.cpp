#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "quilt/quilt_plan.hpp"
#include "quilt/zoom_policy.hpp"

#include <initializer_list>
#include <string>

namespace {

chart_view::runtime::quilt::QuiltLayer makeLayer(
  std::string chartId,
  double nativeScale,
  std::uint32_t drawOrder)
{
  chart_view::runtime::quilt::QuiltLayer layer;
  layer.chartId = std::move(chartId);
  layer.sencPath = layer.chartId + ".senc";
  layer.sourceType = chart_view_chart_source_s57;
  layer.nativeScale = nativeScale;
  layer.usageBand = 5;
  layer.drawOrder = drawOrder;
  layer.fullExtent = {120.0, 30.0, 121.0, 31.0};
  layer.visibleExtent = layer.fullExtent;
  return layer;
}

chart_view::runtime::quilt::QuiltPlan makePlan(std::initializer_list<double> scales)
{
  chart_view::runtime::quilt::QuiltPlan plan;
  plan.setViewport({120.0, 30.0, 121.0, 31.0}, 90000.0);

  std::uint32_t drawOrder = 0;
  for(const auto scale : scales) {
    plan.addLayer(makeLayer("chart" + std::to_string(drawOrder), scale, drawOrder));
    ++drawOrder;
  }

  return plan;
}

}// namespace

TEST_CASE("ZoomPolicy applies deterministic zoom steps with clamping", "[zoom]")
{
  chart_view::runtime::quilt::ZoomPolicy policy;

  REQUIRE(policy.zoomIn(100000.0) == Catch::Approx(80000.0));
  REQUIRE(policy.zoomOut(100000.0) == Catch::Approx(125000.0));
  REQUIRE(policy.zoomIn(900.0) == Catch::Approx(1000.0));
}

TEST_CASE("ZoomPolicy keeps the current plan when the primary chart remains best", "[zoom]")
{
  const auto plan = makePlan({90000.0, 20000.0});
  chart_view::runtime::quilt::ZoomPolicy policy;

  const auto decision = policy.evaluate(plan, 100000.0);

  REQUIRE(decision.preferredLayerIndex == 0);
  REQUIRE(decision.primaryScaleState == chart_view::runtime::quilt::ZoomScaleState::kNormal);
  REQUIRE_FALSE(decision.shouldRebuildPlan);
  REQUIRE(decision.rebuildReason == chart_view::runtime::quilt::ZoomRebuildReason::kNone);
}

TEST_CASE("ZoomPolicy requests rebuild when a different chart becomes the best fit", "[zoom]")
{
  const auto plan = makePlan({90000.0, 20000.0});
  chart_view::runtime::quilt::ZoomPolicy policy;

  const auto decision = policy.evaluate(plan, 35000.0);

  REQUIRE(decision.preferredLayerIndex == 1);
  REQUIRE(decision.shouldRebuildPlan);
  REQUIRE(decision.rebuildReason ==
          chart_view::runtime::quilt::ZoomRebuildReason::kPreferredChartChanged);
}

TEST_CASE("ZoomPolicy reports overzoom and underzoom for single-chart plans", "[zoom]")
{
  const auto plan = makePlan({40000.0});
  chart_view::runtime::quilt::ZoomPolicy policy;

  const auto overzoom = policy.evaluate(plan, 15000.0);
  REQUIRE(overzoom.primaryScaleState == chart_view::runtime::quilt::ZoomScaleState::kOverzoom);
  REQUIRE(overzoom.shouldRebuildPlan);
  REQUIRE(overzoom.rebuildReason == chart_view::runtime::quilt::ZoomRebuildReason::kOverzoom);

  const auto underzoom = policy.evaluate(plan, 120000.0);
  REQUIRE(underzoom.primaryScaleState == chart_view::runtime::quilt::ZoomScaleState::kUnderzoom);
  REQUIRE(underzoom.shouldRebuildPlan);
  REQUIRE(underzoom.rebuildReason == chart_view::runtime::quilt::ZoomRebuildReason::kUnderzoom);
}

TEST_CASE("ZoomPolicy requests rebuild for empty plans", "[zoom]")
{
  chart_view::runtime::quilt::QuiltPlan emptyPlan;
  chart_view::runtime::quilt::ZoomPolicy policy;

  const auto decision = policy.evaluate(emptyPlan, 90000.0);

  REQUIRE(decision.primaryScaleState == chart_view::runtime::quilt::ZoomScaleState::kNoCharts);
  REQUIRE(decision.shouldRebuildPlan);
  REQUIRE(decision.rebuildReason == chart_view::runtime::quilt::ZoomRebuildReason::kEmptyPlan);
}
