#ifndef CHART_VIEW_RUNTIME_CM93_CM93_DECODE_HPP
#define CHART_VIEW_RUNTIME_CM93_CM93_DECODE_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace chart_view::runtime::cm93 {

// ----------------------------------------------------------------
// CM93 v2 XOR decryption
// ----------------------------------------------------------------

// Decrypt a CM93 cell buffer in-place.
// The key is derived from the cell file name (e.g. "00540000.C").
void cm93Decrypt(std::vector<std::uint8_t> &data, const std::string &cellName);

// ----------------------------------------------------------------
// CM93 cell binary layout (post-decryption)
// ----------------------------------------------------------------

// Cell header (first 128 bytes after decryption).
struct Cm93CellHeader
{
  std::uint16_t numObjectClasses{0};
  std::int32_t cellMinX{0}; // easting in CM93 internal units
  std::int32_t cellMinY{0}; // northing
  std::int32_t cellMaxX{0};
  std::int32_t cellMaxY{0};
};

// A single geometry point in CM93 internal coordinates.
struct Cm93Point
{
  std::int32_t x{0};
  std::int32_t y{0};
};

// A decoded feature from a CM93 cell.
struct Cm93Feature
{
  std::uint8_t objectClassIndex{0}; // index into the cell's object class table
  std::uint8_t geometryPrimitive{0}; // 1=point, 2=line, 4=area
  std::vector<std::vector<Cm93Point>> geometry; // rings/parts
  std::vector<std::pair<std::uint16_t, std::string>> attributes; // (attrCode, value)
};

// Decoded cell.
struct Cm93Cell
{
  Cm93CellHeader header;
  std::vector<std::uint16_t> objectClassCodes; // S-57 OBJL codes
  std::vector<Cm93Feature> features;
};

// ----------------------------------------------------------------
// CM93 coordinate transform
// ----------------------------------------------------------------

// Scale factor to convert CM93 internal coords for a given detail level.
double cm93ScaleFactor(char detailLevel);

// Convert CM93 internal coordinates to WGS-84 lon/lat.
void cm93ToLonLat(std::int32_t x, std::int32_t y,
                  double cellOriginLon, double cellOriginLat,
                  double scaleFactor,
                  double &outLon, double &outLat);

// Parse the cell name to extract geographic origin.
// Cell name like "00540000" encodes lat/lon in the naming convention.
bool cm93CellOrigin(const std::string &cellName, char detailLevel,
                    double &outLon, double &outLat);

// ----------------------------------------------------------------
// CM93 cell decoder
// ----------------------------------------------------------------

struct Cm93DecodeResult
{
  bool ok{false};
  std::string error;
  Cm93Cell cell;
};

// Decode a CM93 cell from raw (encrypted) bytes. cellName is the filename
// (e.g. "00540000.C") used for decryption key derivation.
[[nodiscard]] Cm93DecodeResult decodeCm93Cell(
  std::vector<std::uint8_t> data,
  const std::string &cellName);

}// namespace chart_view::runtime::cm93

#endif
