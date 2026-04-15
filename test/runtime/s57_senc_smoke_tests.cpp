#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "s57/s57_reader.hpp"
#include "senc/senc_writer.hpp"
#include "senc/senc_reader.hpp"
#include "chart_data/feature_chart_dataset.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

using namespace chart_view::runtime;
using namespace chart_view::runtime::s57;
using namespace chart_view::runtime::senc;

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

TEST_CASE("S57 -> SENC build and readback", "[s57][senc][smoke][real-data]")
{
  auto root = getS57Root();
  auto chartPath = findFirstChart(root);
  INFO("chart: " << chartPath);

  // Step 1: Read S57 chart.
  S57Reader reader;
  auto readResult = reader.read(chartPath);
  REQUIRE(readResult.ok);
  REQUIRE(readResult.dataset.featureCount() > 0);

  const auto &srcDataset = readResult.dataset;
  const auto srcFeatureCount = srcDataset.featureCount();
  const auto &srcExtent = srcDataset.meta().extent;
  const auto &srcName = srcDataset.meta().name;

  INFO("source features: " << srcFeatureCount);
  INFO("source name: " << srcName);
  INFO("source extent: [" << srcExtent.minLon << "," << srcExtent.minLat
       << " -> " << srcExtent.maxLon << "," << srcExtent.maxLat << "]");

  REQUIRE(srcExtent.isValid());

  // Step 2: Write SENC.
  SencWriter writer;
  auto sencBlob = writer.write(srcDataset);
  REQUIRE(sencBlob.size() > 0);

  INFO("SENC blob size: " << sencBlob.size() << " bytes");

  // Step 3: Read SENC back.
  SencReader sencReader;
  auto sencResult = sencReader.read(sencBlob);
  REQUIRE(sencResult.ok);

  const auto &dstDataset = sencResult.dataset;
  const auto dstFeatureCount = dstDataset.featureCount();
  const auto &dstExtent = dstDataset.meta().extent;
  const auto &dstName = dstDataset.meta().name;

  INFO("readback features: " << dstFeatureCount);
  INFO("readback name: " << dstName);
  INFO("readback extent: [" << dstExtent.minLon << "," << dstExtent.minLat
       << " -> " << dstExtent.maxLon << "," << dstExtent.maxLat << "]");

  // Step 4: Verify identity matches.
  REQUIRE(dstName == srcName);
  REQUIRE(dstFeatureCount == srcFeatureCount);
  REQUIRE(dstExtent.isValid());

  // Extent should match within floating-point tolerance.
  REQUIRE(dstExtent.minLon == Catch::Approx(srcExtent.minLon).margin(1e-6));
  REQUIRE(dstExtent.maxLon == Catch::Approx(srcExtent.maxLon).margin(1e-6));
  REQUIRE(dstExtent.minLat == Catch::Approx(srcExtent.minLat).margin(1e-6));
  REQUIRE(dstExtent.maxLat == Catch::Approx(srcExtent.maxLat).margin(1e-6));

  // Source type should be preserved.
  REQUIRE(dstDataset.meta().sourceType == chart_view_chart_source_s57);

  // Log summary.
  std::cout << "[S57->SENC smoke] " << srcName
            << ": " << srcFeatureCount << " features, "
            << sencBlob.size() << " bytes SENC\n";
}
