#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "s101/s101_reader.hpp"
#include "s101/s101_normalizer.hpp"
#include "chart_data/feature_chart_dataset.hpp"

#include <string>
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

TEST_CASE("S101Reader parses synthetic smoke fixture into renderable features", "[s101]")
{
  S101Reader reader;
  static constexpr auto kSyntheticSmoke = R"(S101SMOKE
name=synthetic_host_smoke
native_scale=12000
point=121.8006,31.2301
line=121.8002,31.2298;121.8013,31.2304;121.8020,31.2299
area=121.8004,31.2300;121.8014,31.2300;121.8014,31.2308;121.8004,31.2308
)";

  const auto bytes = std::vector<std::uint8_t>(kSyntheticSmoke, kSyntheticSmoke + std::char_traits<char>::length(kSyntheticSmoke));
  auto result = reader.readFromMemory(bytes, "ignored_name");

  REQUIRE(result.ok);
  REQUIRE(result.error.empty());
  REQUIRE(result.dataset.meta().name == "synthetic_host_smoke");
  REQUIRE(result.dataset.meta().sourceType == chart_view_chart_source_s101);
  REQUIRE(result.dataset.meta().nativeScale == Catch::Approx(12000.0));
  REQUIRE(result.dataset.featureCount() == 3);
  REQUIRE(result.dataset.meta().extent.isValid());
  REQUIRE(result.dataset.features()[0].classAcronym == "Sounding");
  REQUIRE(result.dataset.features()[1].classAcronym == "DepthContour");
  REQUIRE(result.dataset.features()[2].classAcronym == "DepthArea");
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
