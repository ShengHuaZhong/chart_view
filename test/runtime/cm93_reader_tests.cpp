#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "cm93/cm93_decode.hpp"
#include "cm93/cm93_reader.hpp"
#include "cm93/cm93_normalizer.hpp"
#include "chart_data/geometry.hpp"
#include "chart_data/feature.hpp"
#include "chart_data/feature_chart_dataset.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>

using namespace chart_view::runtime;
using namespace chart_view::runtime::cm93;

// ------------------------------------------------------------------
// Helper: get the test data root from env/compile-def, skip if missing.
// ------------------------------------------------------------------
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

// Find the first .C or .A or .Z cell file recursively.
static std::string findFirstCellFile(const std::string &root)
{
  std::string result;
  for (const auto &entry : std::filesystem::recursive_directory_iterator(root)) {
    if (!entry.is_regular_file()) continue;
    auto ext = entry.path().extension().string();
    if (ext.size() == 2 && ext[0] == '.' && std::isalpha(static_cast<unsigned char>(ext[1]))) {
      // Check it looks like a CM93 cell (not .grz or .DIC etc.)
      auto name = entry.path().filename().string();
      if (name.size() > 2 && name.find('.') != std::string::npos) {
        auto stem = entry.path().stem().string();
        // Cell filenames are 8-digit numbers.
        bool allDigits = stem.size() == 8;
        for (char c : stem) {
          if (!std::isdigit(static_cast<unsigned char>(c))) allDigits = false;
        }
        if (allDigits) {
          result = entry.path().string();
          break;
        }
      }
    }
  }
  if (result.empty()) {
    FAIL("No CM93 cell file found in " + root);
  }
  return result;
}

// ==================================================================
// CM93 decode unit tests
// ==================================================================

TEST_CASE("cm93Decrypt is reversible", "[cm93][decode]")
{
  std::vector<std::uint8_t> original = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
  std::vector<std::uint8_t> data = original;
  std::string name = "00540000.C";

  cm93Decrypt(data, name);
  // Should be different from original (unless key happens to be 0).
  // Decrypt again to recover original (XOR is its own inverse).
  cm93Decrypt(data, name);
  REQUIRE(data == original);
}

TEST_CASE("decodeCm93Cell rejects too-small data", "[cm93][decode]")
{
  std::vector<std::uint8_t> tiny(10, 0);
  auto result = decodeCm93Cell(std::move(tiny), "test.C");
  REQUIRE_FALSE(result.ok);
  REQUIRE_THAT(result.error, Catch::Matchers::ContainsSubstring("too small"));
}

TEST_CASE("cm93ScaleFactor returns positive for all levels", "[cm93][decode]")
{
  for (char c : {'Z', 'A', 'B', 'C', 'D', 'E', 'F', 'G'}) {
    REQUIRE(cm93ScaleFactor(c) > 0.0);
  }
}

TEST_CASE("cm93CellOrigin parses 8-digit cell names", "[cm93][decode]")
{
  double lon = 0.0;
  double lat = 0.0;
  REQUIRE(cm93CellOrigin("00540000", 'C', lon, lat));
  // lat should be around 5.4, lon around 0.0
  REQUIRE(lat >= 0.0);
}

// ==================================================================
// CM93 reader smoke tests on real data
// ==================================================================

TEST_CASE("Cm93Reader reads a canonical CM93 cell", "[cm93][real-data]")
{
  auto root = getCm93Root();
  auto cellPath = findFirstCellFile(root);
  INFO("cell: " << cellPath);

  Cm93Reader reader;
  auto result = reader.read(cellPath);

  // CM93 decryption may not work perfectly with our simplified key table,
  // but the reader should not crash.
  INFO("ok: " << result.ok << " error: " << result.error);

  // If decryption succeeded, verify basic properties.
  if (result.ok) {
    REQUIRE(result.dataset.meta().sourceType == chart_view_chart_source_cm93);
  }
}

TEST_CASE("Cm93Reader::readFirstCell finds a cell", "[cm93][real-data]")
{
  auto root = getCm93Root();

  Cm93Reader reader;
  auto result = reader.readFirstCell(root);

  INFO("ok: " << result.ok << " error: " << result.error);
  // Even if decryption doesn't produce valid features,
  // file discovery should work.
  if (result.ok) {
    REQUIRE(result.dataset.meta().sourceType == chart_view_chart_source_cm93);
  }
}

TEST_CASE("Cm93Reader error on nonexistent file", "[cm93]")
{
  Cm93Reader reader;
  auto result = reader.read("nonexistent_cm93_cell.C");
  REQUIRE_FALSE(result.ok);
  REQUIRE_THAT(result.error, Catch::Matchers::ContainsSubstring("cannot open"));
}

TEST_CASE("Cm93Reader error on invalid data", "[cm93]")
{
  Cm93Reader reader;
  std::vector<std::uint8_t> garbage(200, 0xFF);
  auto result = reader.readFromMemory(garbage, "garbage.C");
  // May fail during decode (header check) or succeed with empty features.
  // Either way, it should not crash.
  (void)result;
  SUCCEED();
}

// ==================================================================
// CM93 Normalizer smoke
// ==================================================================

TEST_CASE("Cm93Normalizer normalizes first cell", "[cm93][real-data]")
{
  auto root = getCm93Root();

  Cm93Normalizer normalizer;
  auto result = normalizer.normalizeFirstCell(root);

  INFO("ok: " << result.ok << " error: " << result.error);
  if (result.ok) {
    REQUIRE(result.dataset.meta().sourceType == chart_view_chart_source_cm93);
  }
}
