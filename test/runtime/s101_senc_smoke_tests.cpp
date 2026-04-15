#include <catch2/catch_test_macros.hpp>

#include "s101/s101_reader.hpp"
#include "senc/senc_writer.hpp"
#include "senc/senc_reader.hpp"
#include "chart_data/feature_chart_dataset.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

using namespace chart_view::runtime;
using namespace chart_view::runtime::s101;
using namespace chart_view::runtime::senc;

TEST_CASE("S101 -> SENC build and readback (scaffold)", "[s101][senc][smoke]")
{
  // S-101 reader is scaffold-only in Phase 1 -- produces 0 features.
  // This test verifies the metadata roundtrip through SENC.
  std::vector<std::uint8_t> fakeData(64, 0x20);

  S101Reader reader;
  auto readResult = reader.readFromMemory(fakeData, "test_s101_chart");
  REQUIRE(readResult.ok);

  const auto &srcDataset = readResult.dataset;
  const auto srcFeatureCount = srcDataset.featureCount();
  const auto &srcName = srcDataset.meta().name;

  REQUIRE(srcName == "test_s101_chart");
  REQUIRE(srcFeatureCount == 0);
  REQUIRE(srcDataset.meta().sourceType == chart_view_chart_source_s101);

  // Write SENC.
  SencWriter writer;
  auto sencBlob = writer.write(srcDataset);
  REQUIRE(sencBlob.size() > 0);

  INFO("SENC blob size: " << sencBlob.size() << " bytes");

  // Read SENC back.
  SencReader sencReader;
  auto sencResult = sencReader.read(sencBlob);
  REQUIRE(sencResult.ok);

  const auto &dstDataset = sencResult.dataset;

  // Verify metadata roundtrip.
  REQUIRE(dstDataset.meta().name == srcName);
  REQUIRE(dstDataset.featureCount() == srcFeatureCount);
  REQUIRE(dstDataset.meta().sourceType == chart_view_chart_source_s101);

  std::cout << "[S101->SENC smoke] scaffold: " << srcName
            << ", " << srcFeatureCount << " features, "
            << sencBlob.size() << " bytes SENC\n";
}
