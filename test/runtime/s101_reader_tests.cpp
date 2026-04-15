#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "s101/s101_reader.hpp"
#include "s101/s101_normalizer.hpp"
#include "chart_data/feature_chart_dataset.hpp"

#include <vector>

using namespace chart_view::runtime;
using namespace chart_view::runtime::s101;

// ==================================================================
// S101Reader scaffold tests
// ==================================================================

TEST_CASE("S101Reader rejects too-small data", "[s101]")
{
  S101Reader reader;
  std::vector<std::uint8_t> tiny(10, 0);
  auto result = reader.readFromMemory(tiny, "test");
  REQUIRE_FALSE(result.ok);
  REQUIRE_THAT(result.error, Catch::Matchers::ContainsSubstring("too small"));
}

TEST_CASE("S101Reader scaffold produces empty dataset", "[s101]")
{
  S101Reader reader;
  // Fake 24-byte minimum data.
  std::vector<std::uint8_t> data(64, 0x20);
  auto result = reader.readFromMemory(data, "test_s101");

  REQUIRE(result.ok);
  REQUIRE(result.dataset.meta().sourceType == chart_view_chart_source_s101);
  REQUIRE(result.dataset.meta().name == "test_s101");
  // Scaffold produces zero features.
  REQUIRE(result.dataset.featureCount() == 0);
}

TEST_CASE("S101Reader error on nonexistent file", "[s101]")
{
  S101Reader reader;
  auto result = reader.read("nonexistent_s101_file.000");
  REQUIRE_FALSE(result.ok);
  REQUIRE_THAT(result.error, Catch::Matchers::ContainsSubstring("cannot open"));
}

TEST_CASE("S101Normalizer scaffold works", "[s101]")
{
  S101Normalizer normalizer;
  std::vector<std::uint8_t> data(64, 0x20);
  auto result = normalizer.normalizeFromMemory(data, "norm_test");

  REQUIRE(result.ok);
  REQUIRE(result.dataset.meta().sourceType == chart_view_chart_source_s101);
}
