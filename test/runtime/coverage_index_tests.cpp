#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "catalog/chart_catalog.hpp"
#include "catalog/coverage_index.hpp"
#include "chart_data/feature.hpp"
#include "chart_data/feature_chart_dataset.hpp"
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
        ("chart_view_coverage_tests_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
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
      80000.0,
      {120.6, 30.4, 121.6, 31.4},
      3),
    "approach");
  writeFixture(
    path / "remote.senc",
    makeDataset(
      "remote",
      chart_view_chart_source_s101,
      120000.0,
      {125.0, 35.0, 126.0, 36.0},
      2),
    "remote");

  chart_view::runtime::catalog::ChartCatalog catalog;
  REQUIRE(catalog.loadDirectory(path));
  return catalog;
}

}// namespace

TEST_CASE("CoverageIndex returns intersecting charts for viewport bbox", "[coverage]")
{
  auto catalog = loadCatalogFixture();

  chart_view::runtime::catalog::CoverageIndex index;
  REQUIRE(index.build(catalog));
  REQUIRE(index.size() == 3);
  REQUIRE(index.bucketCount() > 0);

  const chart_view::runtime::chart_data::Extent viewport{120.5, 30.5, 120.9, 30.9};
  const auto matches = index.query(viewport);

  REQUIRE(matches.size() == 2);
  REQUIRE(matches[0]->id == "approach");
  REQUIRE(matches[1]->id == "harbor");
}

TEST_CASE("CoverageIndex deduplicates charts spanning multiple buckets", "[coverage]")
{
  auto catalog = loadCatalogFixture();

  chart_view::runtime::catalog::CoverageIndex index;
  REQUIRE(index.build(catalog));

  const chart_view::runtime::chart_data::Extent viewport{120.0, 30.0, 121.6, 31.4};
  const auto matches = index.query(viewport);

  REQUIRE(matches.size() == 2);
  REQUIRE(matches[0]->id == "approach");
  REQUIRE(matches[1]->id == "harbor");
}

TEST_CASE("CoverageIndex returns no matches for invalid or remote viewport", "[coverage]")
{
  auto catalog = loadCatalogFixture();

  chart_view::runtime::catalog::CoverageIndex index;
  REQUIRE(index.build(catalog));

  const chart_view::runtime::chart_data::Extent invalidViewport{1.0, 1.0, -1.0, -1.0};
  CHECK(index.query(invalidViewport).empty());

  const chart_view::runtime::chart_data::Extent remoteViewport{110.0, 20.0, 111.0, 21.0};
  CHECK(index.query(remoteViewport).empty());
}
