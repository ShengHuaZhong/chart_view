#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "s57/iso8211.hpp"
#include "s57/s57_reader.hpp"
#include "s57/s57_normalizer.hpp"
#include "s57/s57_semantic_mapping.hpp"
#include "chart_data/geometry.hpp"
#include "chart_data/feature.hpp"
#include "chart_data/feature_chart_dataset.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>

using namespace chart_view::runtime;
using namespace chart_view::runtime::s57;

// ------------------------------------------------------------------
// Helper: get the test data root from env, skip if not available.
// ------------------------------------------------------------------
static std::string getS57Root()
{
#ifdef CHARTSYS_S57_TESTDATA_ROOT
  const char *root = CHARTSYS_S57_TESTDATA_ROOT;
#else
  const char *root = std::getenv("CHARTSYS_S57_TESTDATA_ROOT");
#endif
  if (!root || !std::filesystem::exists(root)) {
    SKIP("CHARTSYS_S57_TESTDATA_ROOT not set or missing");
  }
  return root;
}

// Find first .000 file in the test data root.
static std::string findFirstChart(const std::string &root)
{
  std::string result;
  for (const auto &entry : std::filesystem::directory_iterator(root)) {
    if (entry.path().extension() == ".000") {
      result = entry.path().string();
      break;
    }
  }
  if (result.empty()) {
    FAIL("No .000 file found in " + root);
  }
  return result;
}

static std::string findChartByStem(const std::string &root, std::string_view stem)
{
  for(const auto &entry : std::filesystem::directory_iterator(root)) {
    if(entry.path().extension() == ".000" && entry.path().stem().string() == stem) {
      return entry.path().string();
    }
  }

  return {};
}

// ==================================================================
// ISO 8211 parser unit tests
// ==================================================================

TEST_CASE("iso8211::parse rejects empty data", "[s57][iso8211]")
{
  std::vector<std::uint8_t> empty;
  auto result = iso8211::parse(empty);
  REQUIRE_FALSE(result.ok);
  REQUIRE_THAT(result.error, Catch::Matchers::ContainsSubstring("too small"));
}

TEST_CASE("iso8211::parse rejects truncated data", "[s57][iso8211]")
{
  std::vector<std::uint8_t> data(20, 0x20); // 20 spaces
  auto result = iso8211::parse(data);
  REQUIRE_FALSE(result.ok);
}

// ==================================================================
// S57Reader smoke tests on real chart data
// ==================================================================

TEST_CASE("S57Reader reads canonical S57 chart", "[s57][real-data]")
{
  auto root = getS57Root();
  auto chartPath = findFirstChart(root);

  S57Reader reader;
  auto result = reader.read(chartPath);

  REQUIRE(result.ok);
  REQUIRE(result.error.empty());

  const auto &ds = result.dataset;
  INFO("chart: " << chartPath);
  INFO("features: " << ds.featureCount());
  INFO("name: " << ds.meta().name);

  // Non-zero feature count.
  REQUIRE(ds.featureCount() > 0);

  // Dataset extent must be valid.
  const auto &ext = ds.meta().extent;
  REQUIRE(ext.isValid());

  // Extent should be in reasonable geographic range.
  REQUIRE(ext.minLon >= -180.0);
  REQUIRE(ext.maxLon <= 180.0);
  REQUIRE(ext.minLat >= -90.0);
  REQUIRE(ext.maxLat <= 90.0);

  // Source type must be S-57.
  REQUIRE(ds.meta().sourceType == chart_view_chart_source_s57);
}

TEST_CASE("S57Reader extracts point, line, area features", "[s57][real-data]")
{
  auto root = getS57Root();
  auto chartPath = findFirstChart(root);

  S57Reader reader;
  auto result = reader.read(chartPath);
  REQUIRE(result.ok);

  std::size_t pointCount = 0;
  std::size_t lineCount = 0;
  std::size_t areaCount = 0;

  for (const auto &feat : result.dataset.features()) {
    std::visit([&](const auto &geo) {
      using T = std::decay_t<decltype(geo)>;
      if constexpr (std::is_same_v<T, chart_data::PointGeometry>) {
        ++pointCount;
      } else if constexpr (std::is_same_v<T, chart_data::LineGeometry>) {
        ++lineCount;
      } else if constexpr (std::is_same_v<T, chart_data::AreaGeometry>) {
        ++areaCount;
      }
    }, feat.geometry);
  }

  INFO("points: " << pointCount << " lines: " << lineCount << " areas: " << areaCount);

  // At least some features of each type should exist in a typical ENC chart.
  // But we require at least one category to be non-empty.
  REQUIRE((pointCount + lineCount + areaCount) > 0);
}

TEST_CASE("S57Reader features have attributes", "[s57][real-data]")
{
  auto root = getS57Root();
  auto chartPath = findFirstChart(root);

  S57Reader reader;
  auto result = reader.read(chartPath);
  REQUIRE(result.ok);

  // At least some features should have attributes.
  std::size_t withAttrs = 0;
  for (const auto &feat : result.dataset.features()) {
    if (!feat.attributes.empty()) ++withAttrs;
  }

  INFO("features with attributes: " << withAttrs << " / " << result.dataset.featureCount());
  REQUIRE(withAttrs > 0);
}

TEST_CASE("S57 semantic mapping exposes the Phase 4 baseline acronyms", "[s57][semantic]")
{
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(30) == "COALNE");
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(42) == "DEPARE");
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(43) == "DEPCNT");
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(75) == "LIGHTS");
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(129) == "SOUNDG");
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(159) == "WRECKS");

  REQUIRE(chart_view::runtime::s57::lookupAttributeAcronym(116) == "OBJNAM");
  REQUIRE(chart_view::runtime::s57::lookupAttributeAcronym(301) == "NOBJNM");
  REQUIRE(chart_view::runtime::s57::lookupAttributeAcronym(174) == "VALDCO");
  REQUIRE(chart_view::runtime::s57::lookupAttributeAcronym(179) == "VALSOU");
}

TEST_CASE("S57Reader preserves baseline semantic names for the known real pair", "[s57][real-data][semantic][phase4]")
{
  const auto root = getS57Root();
  const auto chartAPath = findChartByStem(root, "C1511781");
  const auto chartBPath = findChartByStem(root, "C1511782");
  if(chartAPath.empty() || chartBPath.empty()) {
    SKIP("Known real-pair charts C1511781/C1511782 not available");
  }

  S57Reader reader;
  const auto chartA = reader.read(chartAPath);
  const auto chartB = reader.read(chartBPath);
  REQUIRE(chartA.ok);
  REQUIRE(chartB.ok);

  std::size_t mappedClassCount = 0;
  std::size_t namedFeatureCount = 0;
  std::size_t nationalNameCount = 0;
  for(const auto *dataset : {&chartA.dataset, &chartB.dataset}) {
    for(const auto &feature : dataset->features()) {
      if(!feature.classAcronym.empty() && !feature.classAcronym.starts_with("OBJ")) {
        ++mappedClassCount;
      }

      if(feature.attributes.contains("OBJNAM")) {
        ++namedFeatureCount;
      }
      if(feature.attributes.contains("NOBJNM")) {
        ++nationalNameCount;
      }
    }
  }

  INFO("mappedClassCount=" << mappedClassCount);
  INFO("namedFeatureCount=" << namedFeatureCount);
  INFO("nationalNameCount=" << nationalNameCount);

  REQUIRE(mappedClassCount > 0);
  REQUIRE(namedFeatureCount > 0);
}

TEST_CASE("S57Reader error on nonexistent file", "[s57]")
{
  S57Reader reader;
  auto result = reader.read("nonexistent_chart_file.000");
  REQUIRE_FALSE(result.ok);
  REQUIRE_THAT(result.error, Catch::Matchers::ContainsSubstring("cannot open"));
}

TEST_CASE("S57Reader error on invalid data", "[s57]")
{
  S57Reader reader;
  std::vector<std::uint8_t> garbage(100, 0xFF);
  auto result = reader.readFromMemory(garbage, "garbage");
  REQUIRE_FALSE(result.ok);
}

// ==================================================================
// S57Normalizer smoke
// ==================================================================

TEST_CASE("S57Normalizer normalizes canonical chart", "[s57][real-data]")
{
  auto root = getS57Root();
  auto chartPath = findFirstChart(root);

  S57Normalizer normalizer;
  auto result = normalizer.normalize(chartPath);

  REQUIRE(result.ok);
  REQUIRE(result.dataset.featureCount() > 0);
  REQUIRE(result.dataset.meta().extent.isValid());
}
