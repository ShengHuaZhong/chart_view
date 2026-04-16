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
  REQUIRE(plan.selectionResult().orderedChartIds == std::vector<std::string>{
                                                    "modern",
                                                    "approach",
                                                    "harbor"});
  REQUIRE(plan.size() == 3);

  REQUIRE(plan.layers()[0].chartId == "modern");
  REQUIRE(plan.layers()[0].drawOrder == 0);
  REQUIRE(plan.layers()[0].visibleExtent.minLon == Catch::Approx(120.5));
  REQUIRE(plan.layers()[0].visibleExtent.maxLon == Catch::Approx(120.9));

  REQUIRE(plan.layers()[1].chartId == "approach");
  REQUIRE(plan.layers()[1].drawOrder == 1);
  REQUIRE(plan.layers()[1].visibleExtent.maxLat == Catch::Approx(30.9));

  REQUIRE(plan.layers()[2].chartId == "harbor");
  REQUIRE(plan.layers()[2].drawOrder == 2);
  REQUIRE(plan.layers()[2].visibleExtent.maxLon == Catch::Approx(120.8));
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
