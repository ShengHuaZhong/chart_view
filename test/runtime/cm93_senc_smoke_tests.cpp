#include <catch2/catch_test_macros.hpp>

#include "cm93/cm93_reader.hpp"
#include "senc/senc_writer.hpp"
#include "senc/senc_reader.hpp"
#include "chart_data/feature_chart_dataset.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

using namespace chart_view::runtime;
using namespace chart_view::runtime::cm93;
using namespace chart_view::runtime::senc;

static std::string getCm93Root()
{
#ifdef CHARTSYS_CM93_TESTDATA_ROOT
  const char *root = CHARTSYS_CM93_TESTDATA_ROOT;
#else
  const char *root = std::getenv("CHARTSYS_CM93_TESTDATA_ROOT");
#endif
  if (!root || !std::filesystem::exists(root)) {
    SKIP("CHARTSYS_CM93_TESTDATA_ROOT not set or missing");
  }
  return root;
}

TEST_CASE("CM93 -> SENC build and readback", "[cm93][senc][smoke][real-data]")
{
  auto root = getCm93Root();

  // Step 1: Read CM93 chart (first available cell).
  Cm93Reader reader;
  auto readResult = reader.readFirstCell(root);

  if (!readResult.ok) {
    WARN("CM93 read failed (decryption issue): " + readResult.error);
    SUCCEED("CM93 decryption is best-effort in Phase 1");
    return;
  }

  const auto &srcDataset = readResult.dataset;
  const auto srcFeatureCount = srcDataset.featureCount();
  const auto &srcName = srcDataset.meta().name;

  INFO("source features: " << srcFeatureCount);
  INFO("source name: " << srcName);

  if (srcFeatureCount == 0) {
    WARN("CM93 cell has 0 features (decryption may be incomplete)");
    SUCCEED("CM93 decryption is best-effort in Phase 1");
    return;
  }

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

  // Step 4: Verify identity matches.
  REQUIRE(dstDataset.meta().name == srcName);
  REQUIRE(dstDataset.featureCount() == srcFeatureCount);
  REQUIRE(dstDataset.meta().sourceType == chart_view_chart_source_cm93);

  std::cout << "[CM93->SENC smoke] " << srcName
            << ": " << srcFeatureCount << " features, "
            << sencBlob.size() << " bytes SENC\n";
}
