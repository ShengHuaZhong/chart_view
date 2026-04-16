#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "catalog/chart_catalog.hpp"
#include "catalog/chart_selection_policy.hpp"
#include "catalog/coverage_index.hpp"
#include "chart_data/feature.hpp"
#include "chart_data/feature_chart_dataset.hpp"
#include "projection/projected_bounds.hpp"
#include "quilt/quilt_planner.hpp"
#include "senc/source_manifest.hpp"
#include "senc/senc_writer.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

namespace {

class ScopedTempDir
{
public:
  ScopedTempDir()
    : m_path(
        std::filesystem::temp_directory_path() /
        ("chart_view_quilt_planner_tests_" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
  {
    std::filesystem::create_directories(m_path);
  }

  ~ScopedTempDir()
  {
    std::error_code ec;
    std::filesystem::remove_all(m_path, ec);
  }

  [[nodiscard]] const std::filesystem::path &path() const noexcept { return m_path; }

private:
  std::filesystem::path m_path;
};

chart_view::runtime::chart_data::FeatureChartDataset makeDataset(
  const std::string &name,
  chart_view_chart_source_type_t sourceType,
  double nativeScale,
  const chart_view::runtime::chart_data::Extent &extent,
  std::uint32_t usageBand)
{
  using namespace chart_view::runtime::chart_data;

  FeatureChartDataset dataset;
  DatasetMeta meta;
  meta.name = name;
  meta.sourceType = sourceType;
  meta.nativeScale = nativeScale;
  meta.extent = extent;
  meta.usageBand = usageBand;
  dataset.setMeta(meta);

  Feature feature;
  feature.id = 1;
  feature.classCode = 129;
  feature.classAcronym = "SOUNDG";
  feature.geometry = PointGeometry{{extent.minLon, extent.minLat}};
  dataset.addFeature(std::move(feature));

  return dataset;
}

void writeFixture(const std::filesystem::path &path,
                  const chart_view::runtime::chart_data::FeatureChartDataset &dataset,
                  const std::string &manifestName)
{
  chart_view::runtime::senc::SencWriter writer;
  chart_view::runtime::senc::SourceManifest manifest;
  manifest.name = manifestName;
  manifest.sourceType = dataset.meta().sourceType;
  writer.setSourceManifest(manifest);
  REQUIRE(writer.writeToFile(dataset, path.string()));
}

chart_view::runtime::catalog::ChartCatalog loadCatalogFixture()
{
  ScopedTempDir tempDir;
  const auto &path = tempDir.path();

  writeFixture(
    path / "harbor.senc",
    makeDataset(
      "harbor",
      chart_view_chart_source_s57,
      20000.0,
      {120.0, 30.0, 120.8, 30.8},
      5),
    "harbor");
  writeFixture(
    path / "approach.senc",
    makeDataset(
      "approach",
      chart_view_chart_source_cm93,
      90000.0,
      {120.4, 30.4, 121.3, 31.3},
      5),
    "approach");
  writeFixture(
    path / "modern.senc",
    makeDataset(
      "modern",
      chart_view_chart_source_s101,
      90000.0,
      {120.5, 30.5, 121.1, 31.1},
      5),
    "modern");
  writeFixture(
    path / "remote.senc",
    makeDataset(
      "remote",
      chart_view_chart_source_s57,
      500000.0,
      {125.0, 35.0, 126.0, 36.0},
      3),
    "remote");

  chart_view::runtime::catalog::ChartCatalog catalog;
  REQUIRE(catalog.loadDirectory(path));
  return catalog;
}

double patchAreaSum(const chart_view::runtime::quilt::QuiltLayer &layer)
{
  double sum = 0.0;
  for(const auto &patch : layer.projectedPatchExtents) {
    sum += std::max(0.0, patch.maxX - patch.minX) * std::max(0.0, patch.maxY - patch.minY);
  }
  return sum;
}

double visibleArea(const chart_view::runtime::quilt::QuiltLayer &layer)
{
  return std::max(0.0, layer.projectedVisibleExtent.maxX - layer.projectedVisibleExtent.minX)
       * std::max(0.0, layer.projectedVisibleExtent.maxY - layer.projectedVisibleExtent.minY);
}

}// namespace

TEST_CASE("QuiltPlanner builds a stable ordered plan for overlapping charts", "[quilt][planner]")
{
  const auto catalog = loadCatalogFixture();

  chart_view::runtime::catalog::CoverageIndex index;
  REQUIRE(index.build(catalog));

  chart_view::runtime::catalog::ChartSelectionPolicy selectionPolicy;
  chart_view::runtime::quilt::QuiltPlanner planner;

  const auto plan =
    planner.build(index, selectionPolicy, {120.5, 30.5, 120.9, 30.9}, 90000.0);

  REQUIRE_FALSE(plan.empty());
  REQUIRE(plan.viewportScaleDenominator() == Catch::Approx(90000.0));
  REQUIRE(plan.selectionResult().candidateCount == 3);
  REQUIRE(plan.selectionResult().orderedChartIds == std::vector<std::string>{"modern"});
  REQUIRE(plan.size() == 1);

  REQUIRE(plan.layers()[0].chartId == "modern");
  REQUIRE(plan.layers()[0].drawOrder == 0);
  REQUIRE(plan.layers()[0].visibleExtent.minLon == Catch::Approx(120.5));
  REQUIRE(plan.layers()[0].visibleExtent.maxLon == Catch::Approx(120.9));
  REQUIRE(plan.layers()[0].projectedPatchExtents.size() == 1);
}

TEST_CASE("QuiltPlanner keeps viewport state but returns no layers when nothing overlaps", "[quilt][planner]")
{
  const auto catalog = loadCatalogFixture();

  chart_view::runtime::catalog::CoverageIndex index;
  REQUIRE(index.build(catalog));

  chart_view::runtime::catalog::ChartSelectionPolicy selectionPolicy;
  chart_view::runtime::quilt::QuiltPlanner planner;

  const auto plan =
    planner.build(index, selectionPolicy, {110.0, 20.0, 111.0, 21.0}, 150000.0);

  REQUIRE(plan.empty());
  REQUIRE(plan.size() == 0);
  REQUIRE(plan.viewportExtent().minLon == Catch::Approx(110.0));
  REQUIRE(plan.viewportScaleDenominator() == Catch::Approx(150000.0));
  REQUIRE(plan.selectionResult().candidateCount == 0);
  REQUIRE(plan.selectionResult().orderedChartIds.empty());
}

TEST_CASE("QuiltPlanner uses projection-derived viewport extents for high-latitude selection", "[quilt][planner][projection]")
{
  using chart_view::runtime::chart_data::Extent;
  using chart_view::runtime::projection::ProjectionContext;

  ScopedTempDir tempDir;
  const auto &path = tempDir.path();

  writeFixture(
    path / "center.senc",
    makeDataset(
      "center",
      chart_view_chart_source_s57,
      40000.0,
      {-0.10, 79.90, 0.10, 80.10},
      5),
    "center");
  writeFixture(
    path / "far_east.senc",
    makeDataset(
      "far_east",
      chart_view_chart_source_s57,
      40000.0,
      {0.30, 79.90, 0.50, 80.10},
      5),
    "far_east");

  chart_view::runtime::catalog::ChartCatalog catalog;
  REQUIRE(catalog.loadDirectory(path));

  chart_view::runtime::catalog::CoverageIndex index;
  REQUIRE(index.build(catalog));

  chart_view::runtime::catalog::ChartSelectionPolicy selectionPolicy;
  chart_view::runtime::quilt::QuiltPlanner planner;

  chart_view_viewport_t viewport{};
  viewport.center_lon = 0.0;
  viewport.center_lat = 80.0;
  viewport.scale_denominator = 100000.0;
  viewport.pixel_width = 800;
  viewport.pixel_height = 600;

  const auto projectionContext = ProjectionContext::createMercator();
  REQUIRE(projectionContext.isValid());

  Extent viewportExtent{};
  REQUIRE(chart_view::runtime::projection::computeViewportGeographicExtent(
    viewport,
    projectionContext,
    viewportExtent));

  const auto plan = planner.build(index, selectionPolicy, viewportExtent, viewport.scale_denominator);
  REQUIRE_FALSE(plan.empty());
  REQUIRE(plan.size() == 1);
  REQUIRE(plan.layers()[0].chartId == "center");
  REQUIRE(plan.selectionResult().candidateCount == 1);
}

TEST_CASE("QuiltPlanner clips lower-priority projected patches to avoid overlap seams", "[quilt][planner][projection][seam]")
{
  ScopedTempDir tempDir;
  const auto &path = tempDir.path();

  writeFixture(
    path / "primary.senc",
    makeDataset(
      "primary",
      chart_view_chart_source_s57,
      40000.0,
      {0.0, 0.0, 1.0, 1.0},
      5),
    "primary");
  writeFixture(
    path / "secondary.senc",
    makeDataset(
      "secondary",
      chart_view_chart_source_s57,
      80000.0,
      {0.5, 0.0, 1.5, 1.0},
      5),
    "secondary");

  chart_view::runtime::catalog::ChartCatalog catalog;
  REQUIRE(catalog.loadDirectory(path));

  chart_view::runtime::catalog::CoverageIndex index;
  REQUIRE(index.build(catalog));

  chart_view::runtime::catalog::ChartSelectionPolicy selectionPolicy;
  chart_view::runtime::quilt::QuiltPlanner planner;
  const auto plan = planner.build(index, selectionPolicy, {0.0, 0.0, 1.5, 1.0}, 40000.0);

  REQUIRE(plan.size() == 2);
  REQUIRE(plan.layers()[0].chartId == "primary");
  REQUIRE(plan.layers()[1].chartId == "secondary");
  REQUIRE(plan.layers()[0].projectedPatchExtents.size() == 1);
  REQUIRE(plan.layers()[1].projectedPatchExtents.size() == 1);
  REQUIRE(
    plan.layers()[1].projectedPatchExtents.front().minX
    >= plan.layers()[0].projectedPatchExtents.front().maxX);
}

TEST_CASE(
  "QuiltPlanner keeps detail and overview layers for an overlapping projected pair",
  "[quilt][planner][projection][seam][detail-first]")
{
  ScopedTempDir tempDir;
  const auto &path = tempDir.path();

  writeFixture(
    path / "overview.senc",
    makeDataset(
      "overview",
      chart_view_chart_source_s57,
      372284.0,
      {117.55, 38.1597, 118.625, 38.7389},
      3),
    "overview");
  writeFixture(
    path / "detail.senc",
    makeDataset(
      "detail",
      chart_view_chart_source_s57,
      95169.1,
      {117.775, 38.2736, 118.053, 38.4217},
      4),
    "detail");

  chart_view::runtime::catalog::ChartCatalog catalog;
  REQUIRE(catalog.loadDirectory(path));

  chart_view::runtime::catalog::CoverageIndex index;
  REQUIRE(index.build(catalog));

  chart_view::runtime::catalog::ChartSelectionPolicy selectionPolicy;
  chart_view::runtime::quilt::QuiltPlanner planner;
  const auto plan = planner.build(
    index,
    selectionPolicy,
    {117.55, 38.1597, 118.625, 38.7389},
    297827.0);

  REQUIRE(plan.size() == 2);
  REQUIRE(plan.selectionResult().orderedChartIds == std::vector<std::string>{"detail", "overview"});
  REQUIRE(plan.layers()[0].chartId == "detail");
  REQUIRE(plan.layers()[1].chartId == "overview");
  REQUIRE_FALSE(plan.layers()[0].projectedPatchExtents.empty());
  REQUIRE_FALSE(plan.layers()[1].projectedPatchExtents.empty());
  REQUIRE(patchAreaSum(plan.layers()[0]) > 0.0);
  REQUIRE(patchAreaSum(plan.layers()[1]) > 0.0);
  REQUIRE(patchAreaSum(plan.layers()[1]) < visibleArea(plan.layers()[1]));
}
