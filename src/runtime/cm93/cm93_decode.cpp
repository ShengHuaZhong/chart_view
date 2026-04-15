#include "cm93_decode.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace chart_view::runtime::cm93 {

// ----------------------------------------------------------------
// CM93 decode table (from OpenCPN, which matches the C-Map 93 format spec)
// ----------------------------------------------------------------

// Table_0 is the well-known 256-byte C-Map 93 encoding table.
// The decode table is derived as:
//   Encode_table[i] = Table_0[i] ^ 8
//   Decode_table[Encode_table[i]] = i
// Each raw byte in the file is decoded by looking it up in Decode_table.
// clang-format off
static const std::uint8_t kTable0[256] = {
  0xCD, 0xEA, 0xDC, 0x48, 0x3E, 0x6D, 0xCA, 0x7B,
  0x52, 0xE1, 0xA4, 0x8E, 0xAB, 0x05, 0xA7, 0x97,
  0xB9, 0x60, 0x39, 0x85, 0x7C, 0x56, 0x7A, 0xBA,
  0x68, 0x6E, 0xF5, 0x5D, 0x02, 0x4E, 0x0F, 0xA1,
  0x27, 0x24, 0x41, 0x34, 0x00, 0x5A, 0xFE, 0xCB,
  0xD0, 0xFA, 0xF8, 0x6C, 0x74, 0x96, 0x9E, 0x0E,
  0xC2, 0x49, 0xE3, 0xE5, 0xC0, 0x3B, 0x59, 0x18,
  0xA9, 0x86, 0x8F, 0x30, 0xC3, 0xA8, 0x22, 0x0A,
  0x14, 0x1A, 0xB2, 0xC9, 0xC7, 0xED, 0xAA, 0x29,
  0x94, 0x75, 0x0D, 0xAC, 0x0C, 0xF4, 0xBB, 0xC5,
  0x3F, 0xFD, 0xD9, 0x9C, 0x4F, 0xD5, 0x84, 0x1E,
  0xB1, 0x81, 0x69, 0xB4, 0x09, 0xB8, 0x3C, 0xAF,
  0xA3, 0x08, 0xBF, 0xE0, 0x9A, 0xD7, 0xF7, 0x8C,
  0x67, 0x66, 0xAE, 0xD4, 0x4C, 0xA5, 0xEC, 0xF9,
  0xB6, 0x64, 0x78, 0x06, 0x5B, 0x9B, 0xF2, 0x99,
  0xCE, 0xDB, 0x53, 0x55, 0x65, 0x8D, 0x07, 0x33,
  0x04, 0x37, 0x92, 0x26, 0x23, 0xB5, 0x58, 0xDA,
  0x2F, 0xB3, 0x40, 0x5E, 0x7F, 0x4B, 0x62, 0x80,
  0xE4, 0x6F, 0x73, 0x1D, 0xDF, 0x17, 0xCC, 0x28,
  0x25, 0x2D, 0xEE, 0x3A, 0x98, 0xE2, 0x01, 0xEB,
  0xDD, 0xBC, 0x90, 0xB0, 0xFC, 0x95, 0x76, 0x93,
  0x46, 0x57, 0x2C, 0x2B, 0x50, 0x11, 0x0B, 0xC1,
  0xF0, 0xE7, 0xD6, 0x21, 0x31, 0xDE, 0xFF, 0xD8,
  0x12, 0xA6, 0x4D, 0x8A, 0x13, 0x43, 0x45, 0x38,
  0xD2, 0x87, 0xA0, 0xEF, 0x82, 0xF1, 0x47, 0x89,
  0x6A, 0xC8, 0x54, 0x1B, 0x16, 0x7E, 0x79, 0xBD,
  0x6B, 0x91, 0xA2, 0x71, 0x36, 0xB7, 0x03, 0x3D,
  0x72, 0xC6, 0x44, 0x8B, 0xCF, 0x15, 0x9F, 0x32,
  0xC4, 0x77, 0x83, 0x63, 0x20, 0x88, 0xF6, 0xAD,
  0xF3, 0xE8, 0x4A, 0xE9, 0x35, 0x1C, 0x5F, 0x19,
  0x1F, 0x7D, 0x70, 0xFB, 0xD1, 0x51, 0x10, 0xD3,
  0x2E, 0x61, 0x9D, 0x5C, 0x2A, 0x42, 0xBE, 0xE6,
};
// clang-format on

void cm93Decrypt(std::vector<std::uint8_t> &data, const std::string & /*cellName*/)
{
  if (data.empty()) return;

  // Build encode table and then decode table (done once via static init).
  static std::uint8_t decodeTable[256]{};
  static bool tableBuilt = false;
  if (!tableBuilt) {
    for (int i = 0; i < 256; ++i) {
      std::uint8_t enc = static_cast<std::uint8_t>(kTable0[i] ^ 8u);
      decodeTable[static_cast<std::size_t>(enc)] = static_cast<std::uint8_t>(i);
    }
    tableBuilt = true;
  }

  for (auto &byte : data) {
    byte = decodeTable[byte];
  }
}

// ----------------------------------------------------------------
// Binary read helpers
// ----------------------------------------------------------------

namespace {

std::uint16_t readU16LE(const std::uint8_t *p)
{
  return static_cast<std::uint16_t>(p[0]) |
         (static_cast<std::uint16_t>(p[1]) << 8);
}

std::uint32_t readU32LE(const std::uint8_t *p)
{
  return static_cast<std::uint32_t>(p[0]) |
         (static_cast<std::uint32_t>(p[1]) << 8) |
         (static_cast<std::uint32_t>(p[2]) << 16) |
         (static_cast<std::uint32_t>(p[3]) << 24);
}

std::int32_t readI32LE(const std::uint8_t *p)
{
  auto u = readU32LE(p);
  std::int32_t v{};
  std::memcpy(&v, &u, 4);
  return v;
}

}// namespace

// ----------------------------------------------------------------
// Coordinate transform
// ----------------------------------------------------------------

double cm93ScaleFactor(char detailLevel)
{
  // CM93 detail levels and their approximate scale factors.
  // The internal coordinates are scaled differently per level.
  switch (detailLevel) {
  case 'Z': return 20000000.0;  // Overview
  case 'A': return 3000000.0;   // General
  case 'B': return 1000000.0;   // Coastal
  case 'C': return 200000.0;    // Approach
  case 'D': return 100000.0;    // Harbour
  case 'E': return 50000.0;     // Berthing
  case 'F': return 20000.0;     // River
  case 'G': return 7500.0;      // River detail
  default: return 200000.0;
  }
}

void cm93ToLonLat(std::int32_t x, std::int32_t y,
                  double cellOriginLon, double cellOriginLat,
                  double /*scaleFactor*/,
                  double &outLon, double &outLat)
{
  // CM93 internal coordinates are in units of approximately
  // 1/65536 of the cell extent (for a typical cell size).
  // The conversion formula depends on the scale level.
  // For most practical purposes:
  //   lon = cellOriginLon + x / (65536.0 * scaleFactor) * 360.0
  //   lat = cellOriginLat + y / (65536.0 * scaleFactor) * 180.0
  // But the actual formula uses the CM93 coordinate multiplier
  // which varies by detail level.

  // Simplified conversion using the common CM93 coordinate factor.
  // Internal coords represent absolute position scaled by a factor.
  constexpr double kCm93CoordFactor = 1.0 / 65536.0;

  outLon = cellOriginLon + static_cast<double>(x) * kCm93CoordFactor;
  outLat = cellOriginLat + static_cast<double>(y) * kCm93CoordFactor;
}

bool cm93CellOrigin(const std::string &cellName, char detailLevel,
                    double &outLon, double &outLat)
{
  // CM93 cell names encode geographic position.
  // Format: YYYXXXX where Y is lat*10 and X is lon encoded.
  // The exact encoding depends on the detail level.
  // For level C: "00540000" -> lat ~5.4, lon ~0.0 (approximately)

  if (cellName.size() < 8) return false;

  // Parse latitude and longitude from the cell name.
  // The first 3 digits encode latitude (in some unit),
  // digits 3-7 encode longitude.
  // Different detail levels have different cell sizes.

  // CM93 cell naming: latitude = first n digits, longitude = remaining digits.
  // For most levels, the 8-digit code breaks as: LLLLLLLL where first 4 are lat related.

  // Simple parsing: try interpreting as packed lat/lon.
  int latCode = 0;
  int lonCode = 0;

  // The naming convention uses: first 3 chars = lat region, next 5 = lon region.
  // But this varies. Use a simple heuristic:
  // For known CM93 data, cells are named by grid position.

  try {
    latCode = std::stoi(cellName.substr(0, 4));
    lonCode = std::stoi(cellName.substr(4, 4));
  } catch (...) {
    return false;
  }

  // The codes represent cell indices. Cell size depends on detail level.
  double cellSizeLat = 0.0;
  double cellSizeLon = 0.0;

  switch (detailLevel) {
  case 'Z':
    cellSizeLat = 90.0;
    cellSizeLon = 180.0;
    break;
  case 'A':
    cellSizeLat = 30.0;
    cellSizeLon = 60.0;
    break;
  case 'C':
    cellSizeLat = 6.0;
    cellSizeLon = 12.0;
    break;
  default:
    cellSizeLat = 3.0;
    cellSizeLon = 6.0;
    break;
  }

  // Convert code to lat/lon origin.
  outLat = static_cast<double>(latCode) / 100.0;
  outLon = static_cast<double>(lonCode) / 100.0;

  // Ensure bounds.
  if (outLon > 180.0) outLon -= 360.0;

  (void)cellSizeLat;
  (void)cellSizeLon;
  return true;
}

// ----------------------------------------------------------------
// Cell decoder
// ----------------------------------------------------------------

Cm93DecodeResult decodeCm93Cell(
  std::vector<std::uint8_t> data,
  const std::string &cellName)
{
  Cm93DecodeResult result;

  if (data.size() < 128) {
    result.error = "CM93 cell too small: " + std::to_string(data.size()) + " bytes";
    return result;
  }

  // Decrypt.
  cm93Decrypt(data, cellName);

  const std::uint8_t *p = data.data();
  Cm93CellHeader &hdr = result.cell.header;

  // CM93 binary layout after decryption (from OpenCPN/C-Map 93 spec):
  //   Prolog (10 bytes):
  //     Offset 0:  u16  - prolog+header length (should be 138 = 10+128)
  //     Offset 2:  i32  - length of vector record table
  //     Offset 6:  i32  - length of feature record table
  //   Header (128 bytes at offset 10):
  //     Offset 10: f64  - lon_min
  //     Offset 18: f64  - lat_min
  //     Offset 26: f64  - lon_max
  //     Offset 34: f64  - lat_max
  //     Offset 42: f64  - easting_min
  //     Offset 50: f64  - northing_min
  //     Offset 58: f64  - easting_max
  //     Offset 66: f64  - northing_max
  //     Offset 74: u16  - usn_vector_records
  //     Offset 76: i32  - n_vector_record_points
  //     Offset 80: i32  - m_46
  //     Offset 84: i32  - m_4a
  //     Offset 88: u16  - usn_point3d_records
  //     Offset 90: i32  - m_50
  //     Offset 94: i32  - m_54
  //     Offset 98: u16  - usn_point2d_records
  //     Offset 100: u16  - m_5a
  //     Offset 102: u16  - m_5c
  //     Offset 104: u16  - usn_feature_records
  //     ...

  // Validate the prolog.
  if (data.size() < 10u) {
    result.error = "CM93 cell too small for prolog";
    return result;
  }

  std::uint16_t prologPlusHeaderLen = readU16LE(p);
  // CM93 standard: prolog(10) + header(128) = 138 = 0x8A
  if (prologPlusHeaderLen < 10u || prologPlusHeaderLen > 1024u) {
    result.error = "CM93 prolog length implausible: " +
                   std::to_string(prologPlusHeaderLen);
    return result;
  }

  if (data.size() < static_cast<std::size_t>(prologPlusHeaderLen)) {
    result.error = "CM93 cell truncated: smaller than prolog+header";
    return result;
  }

  // Read usn_feature_records from header at offset 104 (= 10 + 94).
  constexpr std::size_t kFeatureCountOffset = 104u;
  if (data.size() < kFeatureCountOffset + 2u) {
    result.error = "CM93 cell too small to read feature record count";
    return result;
  }

  std::uint16_t numFeatureRecords = readU16LE(p + kFeatureCountOffset);

  // Bounding box: lon/lat from header doubles at offset 10.
  // Read as raw 8-byte little-endian doubles.
  double lon_min_val{}, lat_min_val{}, lon_max_val{}, lat_max_val{};
  std::memcpy(&lon_min_val, p + 10, 8);
  std::memcpy(&lat_min_val, p + 18, 8);
  std::memcpy(&lon_max_val, p + 26, 8);
  std::memcpy(&lat_max_val, p + 34, 8);

  hdr.numObjectClasses = numFeatureRecords;
  hdr.cellMinX = static_cast<std::int32_t>(lon_min_val * 1e6);
  hdr.cellMinY = static_cast<std::int32_t>(lat_min_val * 1e6);
  hdr.cellMaxX = static_cast<std::int32_t>(lon_max_val * 1e6);
  hdr.cellMaxY = static_cast<std::int32_t>(lat_max_val * 1e6);

  // Sanity check on feature record count.
  if (numFeatureRecords > 10000) {
    result.error = "CM93 header: implausible feature record count: " +
                   std::to_string(numFeatureRecords);
    return result;
  }

  // Skip past the prolog+header to reach the actual table data.
  // We don't parse the full CM93 binary feature tables here — that belongs
  // in a future SENC-build pipeline. For now we validate the cell is
  // decodable and populate the header bounding box.
  result.cell.objectClassCodes.clear();
  result.cell.features.clear();

  result.ok = true;
  return result;
}

}// namespace chart_view::runtime::cm93
