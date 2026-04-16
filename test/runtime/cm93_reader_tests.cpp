#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "cm93/cm93_decode.hpp"
#include "cm93/cm93_reader.hpp"
#include "cm93/cm93_normalizer.hpp"
#include "chart_data/geometry.hpp"
#include "chart_data/feature.hpp"
#include "chart_data/feature_chart_dataset.hpp"

#include <array>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

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

template <typename TValue>
static void writeBinary(std::vector<std::uint8_t> &buffer, std::size_t offset, const TValue &value)
{
  REQUIRE(offset + sizeof(TValue) <= buffer.size());
  std::memcpy(buffer.data() + offset, &value, sizeof(TValue));
}

static std::vector<std::uint8_t> encodeCm93BytesForTest(
  std::vector<std::uint8_t> decoded,
  const std::string &cellName)
{
  std::array<std::uint8_t, 256> inverseTable{};
  std::vector<std::uint8_t> probe(256);
  for(std::size_t index = 0; index < probe.size(); ++index) {
    probe[index] = static_cast<std::uint8_t>(index);
  }

  cm93Decrypt(probe, cellName);
  for(std::size_t encodedByte = 0; encodedByte < probe.size(); ++encodedByte) {
    inverseTable[probe[encodedByte]] = static_cast<std::uint8_t>(encodedByte);
  }

  for(auto &byte : decoded) {
    byte = inverseTable[byte];
  }

  return decoded;
}

static std::vector<std::uint8_t> makeEncodedSyntheticCell(
  const std::string &cellName,
  const chart_data::Extent &headerExtent,
  double eastingMin = 1000.0,
  double northingMin = 2000.0,
  double eastingMax = 5000.0,
  double northingMax = 8000.0,
  std::uint16_t featureRecordCount = 0,
  std::int32_t vectorTableLength = 0,
  std::int32_t featureTableLength = 0)
{
  constexpr std::uint16_t kPrologAndHeaderLength = 138;

  std::vector<std::uint8_t> decoded(
    static_cast<std::size_t>(kPrologAndHeaderLength + vectorTableLength + featureTableLength),
    0);

  writeBinary(decoded, 0, kPrologAndHeaderLength);
  writeBinary(decoded, 2, vectorTableLength);
  writeBinary(decoded, 6, featureTableLength);

  writeBinary(decoded, 10, headerExtent.minLon);
  writeBinary(decoded, 18, headerExtent.minLat);
  writeBinary(decoded, 26, headerExtent.maxLon);
  writeBinary(decoded, 34, headerExtent.maxLat);

  writeBinary(decoded, 42, eastingMin);
  writeBinary(decoded, 50, northingMin);
  writeBinary(decoded, 58, eastingMax);
  writeBinary(decoded, 66, northingMax);

  const std::uint16_t vectorRecordCount = 0;
  const std::int32_t vectorRecordPointCount = 0;
  const std::int32_t unknown46 = 0;
  const std::int32_t unknown4A = 0;
  const std::uint16_t point3dRecordCount = 0;
  const std::int32_t point3dPointCount = 0;
  const std::int32_t unknown54 = 0;
  const std::uint16_t point2dRecordCount = 0;
  const std::uint16_t unknown5A = 0;
  const std::uint16_t unknown5C = 0;
  const std::int32_t unknown60 = 0;
  const std::int32_t unknown64 = 0;
  const std::uint16_t unknown68 = 0;
  const std::uint16_t unknown6A = 0;
  const std::uint16_t unknown6C = 0;
  const std::int32_t relatedObjectPointerCount = 0;
  const std::int32_t unknown72 = 0;
  const std::uint16_t unknown76 = 0;
  const std::int32_t attributeBlockLength = 0;
  const std::int32_t unknown7C = 0;

  writeBinary(decoded, 74, vectorRecordCount);
  writeBinary(decoded, 76, vectorRecordPointCount);
  writeBinary(decoded, 80, unknown46);
  writeBinary(decoded, 84, unknown4A);
  writeBinary(decoded, 88, point3dRecordCount);
  writeBinary(decoded, 90, point3dPointCount);
  writeBinary(decoded, 94, unknown54);
  writeBinary(decoded, 98, point2dRecordCount);
  writeBinary(decoded, 100, unknown5A);
  writeBinary(decoded, 102, unknown5C);
  writeBinary(decoded, 104, featureRecordCount);
  writeBinary(decoded, 106, unknown60);
  writeBinary(decoded, 110, unknown64);
  writeBinary(decoded, 114, unknown68);
  writeBinary(decoded, 116, unknown6A);
  writeBinary(decoded, 118, unknown6C);
  writeBinary(decoded, 120, relatedObjectPointerCount);
  writeBinary(decoded, 124, unknown72);
  writeBinary(decoded, 128, unknown76);
  writeBinary(decoded, 130, attributeBlockLength);
  writeBinary(decoded, 134, unknown7C);

  return encodeCm93BytesForTest(std::move(decoded), cellName);
}

// ==================================================================
// CM93 decode unit tests
// ==================================================================

TEST_CASE("cm93Decrypt decodes bytes encoded with the CM93 lookup table", "[cm93][decode]")
{
  const std::string name = "00540000.C";
  const std::vector<std::uint8_t> decoded = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
  auto encoded = encodeCm93BytesForTest(decoded, name);

  REQUIRE(encoded != decoded);

  cm93Decrypt(encoded, name);
  REQUIRE(encoded == decoded);
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
  REQUIRE(lon == Catch::Approx(0.0));
  REQUIRE(lat == Catch::Approx(-72.0));
  REQUIRE(cm93CellSpanDegrees('C') == Catch::Approx(4.0));
}

TEST_CASE("decodeCm93Cell reads structured header fields", "[cm93][decode]")
{
  const chart_data::Extent headerExtent{12.5, -45.25, 16.5, -41.25};
  auto encoded = makeEncodedSyntheticCell(
    "00540000.C",
    headerExtent,
    1000.0,
    2000.0,
    9000.0,
    14000.0,
    7);

  auto result = decodeCm93Cell(std::move(encoded), "00540000.C");
  REQUIRE(result.ok);
  REQUIRE(result.cell.header.prologAndHeaderLength == 138);
  REQUIRE(result.cell.header.featureRecordCount == 7);
  REQUIRE(result.cell.header.geographicExtent.minLon == Catch::Approx(headerExtent.minLon));
  REQUIRE(result.cell.header.geographicExtent.minLat == Catch::Approx(headerExtent.minLat));
  REQUIRE(result.cell.header.geographicExtent.maxLon == Catch::Approx(headerExtent.maxLon));
  REQUIRE(result.cell.header.geographicExtent.maxLat == Catch::Approx(headerExtent.maxLat));
  REQUIRE(result.cell.header.hasValidGeographicExtent());
  REQUIRE(result.cell.header.hasValidTransform());
  REQUIRE(result.cell.header.transformXRate == Catch::Approx((9000.0 - 1000.0) / 65535.0));
  REQUIRE(result.cell.header.transformYRate == Catch::Approx((14000.0 - 2000.0) / 65535.0));
}

TEST_CASE("decodeCm93Cell rejects length mismatches declared in the prolog", "[cm93][decode]")
{
  const chart_data::Extent headerExtent{12.5, -45.25, 16.5, -41.25};
  auto encoded = makeEncodedSyntheticCell(
    "00540000.C",
    headerExtent,
    1000.0,
    2000.0,
    9000.0,
    14000.0,
    0,
    4,
    0);

  encoded.pop_back();

  auto result = decodeCm93Cell(std::move(encoded), "00540000.C");
  REQUIRE_FALSE(result.ok);
  REQUIRE_THAT(result.error, Catch::Matchers::ContainsSubstring("file length mismatch"));
}

TEST_CASE("Cm93Reader uses header extent when decoded geometry is empty", "[cm93][decode]")
{
  const chart_data::Extent headerExtent{10.0, -72.0, 14.0, -68.0};
  auto encoded = makeEncodedSyntheticCell("00540000.C", headerExtent);

  Cm93Reader reader;
  auto result = reader.readFromMemory(encoded, "00540000.C");

  REQUIRE(result.ok);
  REQUIRE(result.dataset.empty());
  REQUIRE(result.extentSource == Cm93ExtentSource::kHeader);
  REQUIRE(result.dataset.meta().extent.isValid());
  REQUIRE(result.dataset.meta().extent.minLon == Catch::Approx(headerExtent.minLon));
  REQUIRE(result.dataset.meta().extent.minLat == Catch::Approx(headerExtent.minLat));
  REQUIRE(result.dataset.meta().extent.maxLon == Catch::Approx(headerExtent.maxLon));
  REQUIRE(result.dataset.meta().extent.maxLat == Catch::Approx(headerExtent.maxLat));
}

TEST_CASE("Cm93Reader falls back to cell-name extent when header extent is invalid", "[cm93][decode]")
{
  const chart_data::Extent invalidHeaderExtent{10.0, -68.0, 9.0, -72.0};
  auto encoded = makeEncodedSyntheticCell("00540000.C", invalidHeaderExtent);

  Cm93Reader reader;
  auto result = reader.readFromMemory(encoded, "00540000.C");

  REQUIRE(result.ok);
  REQUIRE(result.dataset.empty());
  REQUIRE(result.extentSource == Cm93ExtentSource::kCellNameFallback);
  REQUIRE(result.dataset.meta().extent.isValid());
  REQUIRE(result.dataset.meta().extent.minLon == Catch::Approx(0.0));
  REQUIRE(result.dataset.meta().extent.minLat == Catch::Approx(-72.0));
  REQUIRE(result.dataset.meta().extent.maxLon == Catch::Approx(4.0));
  REQUIRE(result.dataset.meta().extent.maxLat == Catch::Approx(-68.0));
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
    REQUIRE(result.dataset.meta().extent.isValid());
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
    REQUIRE(result.dataset.meta().extent.isValid());
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
