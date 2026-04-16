#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "catalog/chart_catalog.hpp"
#include "chart_data/feature.hpp"
#include "chart_data/feature_chart_dataset.hpp"
#include "senc/senc_reader.hpp"
#include "senc/senc_writer.hpp"
#include "senc/source_manifest.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace {

class ScopedTempDir
{
public:
  ScopedTempDir()
    : m_path(
        std::filesystem::temp_directory_path() /
        ("chart_view_catalog_tests_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
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
  meta.edition = 2;
  meta.update = 5;
  dataset.setMeta(meta);

  Feature feature;
  feature.id = 1;
  feature.classCode = 129;
  feature.classAcronym = "SOUNDG";
  feature.geometry = PointGeometry{{extent.minLon, extent.minLat}};
  dataset.addFeature(std::move(feature));

  return dataset;
}

void writeCatalogFixture(const std::filesystem::path &path,
                         const chart_view::runtime::chart_data::FeatureChartDataset &dataset,
                         const std::string &manifestName)
{
  chart_view::runtime::senc::SencWriter writer;

  chart_view::runtime::senc::SourceManifest manifest;
  manifest.name = manifestName;
  manifest.sourceType = dataset.meta().sourceType;
  manifest.edition = dataset.meta().edition;
  manifest.update = dataset.meta().update;
  writer.setSourceManifest(manifest);

  REQUIRE(writer.writeToFile(dataset, path.string()));
}

}// namespace

TEST_CASE("SencReader catalog metadata path returns manifest and dataset meta", "[catalog][senc]")
{
  using namespace chart_view::runtime::chart_data;

  auto dataset = makeDataset(
    "dataset_name",
    chart_view_chart_source_s57,
    50000.0,
    {120.1, 30.2, 120.9, 30.8},
    4);

  chart_view::runtime::senc::SencWriter writer;
  chart_view::runtime::senc::SourceManifest manifest;
  manifest.name = "C1511781";
  manifest.sourceType = chart_view_chart_source_s57;
  writer.setSourceManifest(manifest);

  const auto blob = writer.write(dataset);
  REQUIRE_FALSE(blob.empty());

  chart_view::runtime::senc::SencReader reader;
  const auto result = reader.readCatalogMeta(blob);

  REQUIRE(result.ok);
  REQUIRE(result.error.empty());
  REQUIRE(result.manifest.has_value());
  REQUIRE(result.manifest->name == "C1511781");
  REQUIRE(result.meta.name == "dataset_name");
  REQUIRE(result.meta.sourceType == chart_view_chart_source_s57);
  REQUIRE(result.meta.nativeScale == Catch::Approx(50000.0));
  REQUIRE(result.meta.extent.maxLat == Catch::Approx(30.8));
}

TEST_CASE("ChartCatalog loads SENC directory and lists entries by id and metadata", "[catalog]")
{
  ScopedTempDir tempDir;

  const auto alphaPath = tempDir.path() / "alpha.senc";
  const auto bravoPath = tempDir.path() / "bravo.SENC";
  const auto ignoredPath = tempDir.path() / "readme.txt";

  writeCatalogFixture(
    alphaPath,
    makeDataset(
      "alpha_meta",
      chart_view_chart_source_cm93,
      25000.0,
      {121.0, 31.0, 121.4, 31.3},
      5),
    "alpha_id");
  writeCatalogFixture(
    bravoPath,
    makeDataset(
      "bravo_meta",
      chart_view_chart_source_s57,
      90000.0,
      {122.0, 32.0, 122.6, 32.4},
      3),
    "bravo_id");

  {
    std::ofstream ofs(ignoredPath);
    ofs << "ignore";
  }

  chart_view::runtime::catalog::ChartCatalog catalog;
  REQUIRE(catalog.loadDirectory(tempDir.path()));
  REQUIRE(catalog.lastError().empty());
  REQUIRE(catalog.size() == 2);

  const auto &entries = catalog.entries();
  REQUIRE(entries[0].id == "alpha_id");
  REQUIRE(entries[0].sourceType == chart_view_chart_source_cm93);
  REQUIRE(entries[0].nativeScale == Catch::Approx(25000.0));
  REQUIRE(entries[0].extent.minLon == Catch::Approx(121.0));

  REQUIRE(entries[1].id == "bravo_id");
  REQUIRE(entries[1].sourceType == chart_view_chart_source_s57);
  REQUIRE(entries[1].nativeScale == Catch::Approx(90000.0));
  REQUIRE(entries[1].extent.maxLat == Catch::Approx(32.4));

  const auto *alphaEntry = catalog.findById("alpha_id");
  REQUIRE(alphaEntry != nullptr);
  REQUIRE(alphaEntry->usageBand == 5);
}

TEST_CASE("ChartCatalog rejects missing directory", "[catalog]")
{
  chart_view::runtime::catalog::ChartCatalog catalog;
  const auto missingPath =
    std::filesystem::temp_directory_path() / "chart_view_catalog_missing_directory";

  REQUIRE_FALSE(catalog.loadDirectory(missingPath));
  REQUIRE_FALSE(catalog.lastError().empty());
}
