#include "s57_reader.hpp"
#include "iso8211.hpp"
#include "s57_semantic_mapping.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <unordered_map>

namespace chart_view::runtime::s57 {

namespace {

// S-57 record field tags.
constexpr const char *kDSSI = "DSSI"; // Dataset Structure Information
constexpr const char *kDSID = "DSID"; // Dataset Identification
constexpr const char *kDSPM = "DSPM"; // Dataset Parameter
constexpr const char *kFRID = "FRID"; // Feature Record Identifier
constexpr const char *kFOID = "FOID"; // Feature Object Identifier
constexpr const char *kATTF = "ATTF"; // Feature Record Attribute
constexpr const char *kNATF = "NATF"; // Feature Record National Attribute
constexpr const char *kFSPT = "FSPT"; // Feature-to-Spatial pointer
constexpr const char *kVRID = "VRID"; // Vector Record Identifier
constexpr const char *kSG2D = "SG2D"; // 2-D coordinate (vector)
constexpr const char *kSG3D = "SG3D"; // 3-D coordinate (vector)

// S-57 geometry primitives (FRID PRIM values).
enum class S57Prim : std::uint8_t
{
  kPoint = 1,
  kLine = 2,
  kArea = 3,
  kNone = 255
};

// Find a field by tag in a record.
const iso8211::Field *findField(const iso8211::Record &rec, const std::string &tag)
{
  for (const auto &f : rec.fields) {
    if (f.tag == tag) return &f;
  }
  return nullptr;
}

// Find a field definition by tag.
const iso8211::FieldDefinition *findFieldDef(
  const std::vector<iso8211::FieldDefinition> &defs,
  const std::string &tag)
{
  for (const auto &d : defs) {
    if (d.tag == tag) return &d;
  }
  return nullptr;
}

// Read a little-endian uint16 from raw bytes.
std::uint16_t readU16LE(const std::uint8_t *p)
{
  return static_cast<std::uint16_t>(p[0]) |
         (static_cast<std::uint16_t>(p[1]) << 8);
}

// Read a little-endian uint32 from raw bytes.
std::uint32_t readU32LE(const std::uint8_t *p)
{
  return static_cast<std::uint32_t>(p[0]) |
         (static_cast<std::uint32_t>(p[1]) << 8) |
         (static_cast<std::uint32_t>(p[2]) << 16) |
         (static_cast<std::uint32_t>(p[3]) << 24);
}

// Read a little-endian int32 from raw bytes.
std::int32_t readI32LE(const std::uint8_t *p)
{
  std::uint32_t u = readU32LE(p);
  std::int32_t val{};
  std::memcpy(&val, &u, 4);
  return val;
}

// Extract a unit-terminated or field-terminated ASCII subfield from data.
// Advances pos past the terminator.
std::string extractAsciiSubfield(const std::vector<std::uint8_t> &data, std::size_t &pos)
{
  std::size_t start = pos;
  while (pos < data.size() && data[pos] != 0x1F && data[pos] != 0x1E) ++pos;
  std::string val(reinterpret_cast<const char *>(data.data() + start), pos - start);
  if (pos < data.size()) ++pos; // skip terminator
  return val;
}

// Parse DSPM (dataset parameters) to get coordinate multiplication factor.
double parseCoordinateMultiplier(const iso8211::Record &rec)
{
  const auto *dsprField = findField(rec, kDSPM);
  if (!dsprField || dsprField->data.size() < 20) return 1.0e-7;

  // DSPM subfields: RCNM, RCID, HDAT, VDAT, SDAT, CSCL, DUNI, HUNI, PUNI, COUN, COMF, SOMF
  // We need COMF (Coordinate Multiplication Factor) at a specific offset.
  // S-57 spec: COMF is at offset 16 (4 bytes, unsigned).
  // However, the exact offset depends on the field structure.
  // For most S-57 files, COMF is typically 10000000 (1e7) meaning
  // coordinates are stored as int32 * 1e-7 = degrees.

  // Try to find COMF by scanning -- it's typically the first large value
  // at a consistent position. In practice, DSPM has:
  //   RCNM(1) + RCID(4) + HDAT(3) + VDAT(3) + SDAT(3) + CSCL(4)
  //   + DUNI(1) + HUNI(1) + PUNI(1) + COUN(1)
  //   = 22 bytes before COMF
  // But initial subfields may be terminated differently.
  // Let's use byte 20 for COMF (safe for ENC files).
  const auto &d = dsprField->data;

  // Skip past subfield terminators. Look for COMF position by pattern.
  // COMF is a B(4,4) field in most S-57 implementations.
  // We scan from byte 8 onwards for a 4-byte value that looks like a multiplier.
  for (std::size_t p = 8; p + 4 <= d.size(); ++p) {
    if (d[p] == 0x1F || d[p] == 0x1E) continue;
    std::uint32_t candidate = readU32LE(d.data() + p);
    if (candidate == 10000000u || candidate == 1000000u || candidate == 100000u) {
      return 1.0 / static_cast<double>(candidate);
    }
  }

  return 1.0e-7; // default S-57 coordinate factor
}

// Parse all vector records to build a coordinate lookup table.
struct VectorRecord
{
  std::uint32_t rcid{0};
  std::uint8_t rcnm{0}; // 110=VI(isolated node), 120=VC(conn node), 130=VE(edge)
  std::vector<chart_data::Coordinate> coords;
};

std::unordered_map<std::uint64_t, VectorRecord> parseVectorRecords(
  const iso8211::Module &mod,
  double comf)
{
  std::unordered_map<std::uint64_t, VectorRecord> vectors;

  for (const auto &rec : mod.dataRecords) {
    const auto *vridField = findField(rec, kVRID);
    if (!vridField || vridField->data.empty()) continue;

    VectorRecord vr;
    vr.rcnm = vridField->data[0];
    if (vridField->data.size() >= 5) {
      vr.rcid = readU32LE(vridField->data.data() + 1);
    }

    // Parse SG2D (2-D coordinates).
    const auto *sg2d = findField(rec, kSG2D);
    if (sg2d && sg2d->data.size() >= 8) {
      const std::size_t coordSize = 8; // 2 x int32
      std::size_t numCoords = sg2d->data.size() / coordSize;
      vr.coords.reserve(numCoords);
      for (std::size_t i = 0; i < numCoords; ++i) {
        const std::uint8_t *p = sg2d->data.data() + i * coordSize;
        double lat = static_cast<double>(readI32LE(p)) * comf;
        double lon = static_cast<double>(readI32LE(p + 4)) * comf;
        vr.coords.push_back({lon, lat});
      }
    }

    // Parse SG3D (3-D coordinates with depth).
    const auto *sg3d = findField(rec, kSG3D);
    if (sg3d && sg3d->data.size() >= 12) {
      const std::size_t coordSize = 12; // 2 x int32 + int32(depth)
      std::size_t numCoords = sg3d->data.size() / coordSize;
      vr.coords.reserve(numCoords);
      for (std::size_t i = 0; i < numCoords; ++i) {
        const std::uint8_t *p = sg3d->data.data() + i * coordSize;
        double lat = static_cast<double>(readI32LE(p)) * comf;
        double lon = static_cast<double>(readI32LE(p + 4)) * comf;
        vr.coords.push_back({lon, lat});
      }
    }

    // Key by RCNM << 32 | RCID.
    std::uint64_t key = (static_cast<std::uint64_t>(vr.rcnm) << 32) | vr.rcid;
    vectors[key] = std::move(vr);
  }

  return vectors;
}

// Build feature geometry from FSPT references.
chart_data::Geometry buildGeometry(
  S57Prim prim,
  const iso8211::Field *fsptField,
  const std::unordered_map<std::uint64_t, VectorRecord> &vectors)
{
  if (prim == S57Prim::kPoint) {
    // For point features, look at FSPT for isolated node reference.
    if (fsptField && fsptField->data.size() >= 8) {
      // FSPT: NAME(8 bytes) + ORNT(1) + USAG(1) + MASK(1) per entry.
      // NAME is: RCNM(1) + RCID(4) = 5 bytes, but encoded as 8 bytes with
      // additional info. In S-57, FSPT NAME is just a record pointer.
      // Layout: NAME(8) + ORNT(1) + USAG(1) + MASK(1) = 11 bytes per entry.
      // But different implementations vary. Try 8-byte NAME.

      const auto &d = fsptField->data;
      // Try to extract RCNM + RCID from the spatial pointer.
      std::uint8_t rcnm = d[0];
      std::uint32_t rcid = readU32LE(d.data() + 1);
      std::uint64_t key = (static_cast<std::uint64_t>(rcnm) << 32) | rcid;

      auto it = vectors.find(key);
      if (it != vectors.end() && !it->second.coords.empty()) {
        return chart_data::PointGeometry{it->second.coords[0]};
      }
    }
    return chart_data::PointGeometry{};
  }

  if (prim == S57Prim::kLine) {
    chart_data::LineGeometry line;
    if (fsptField && !fsptField->data.empty()) {
      // Extract all spatial references and concatenate coordinates.
      const auto &d = fsptField->data;
      const std::size_t entrySize = 8; // RCNM(1)+RCID(4)+ORNT(1)+USAG(1)+MASK(1)
      std::size_t numEntries = d.size() / entrySize;

      for (std::size_t i = 0; i < numEntries; ++i) {
        const std::uint8_t *p = d.data() + i * entrySize;
        std::uint8_t rcnm = p[0];
        std::uint32_t rcid = readU32LE(p + 1);
        std::uint64_t key = (static_cast<std::uint64_t>(rcnm) << 32) | rcid;

        auto it = vectors.find(key);
        if (it != vectors.end()) {
          for (const auto &c : it->second.coords) {
            line.vertices.push_back(c);
          }
        }
      }
    }
    return line;
  }

  // Area
  chart_data::AreaGeometry area;
  if (fsptField && !fsptField->data.empty()) {
    const auto &d = fsptField->data;
    const std::size_t entrySize = 8;
    std::size_t numEntries = d.size() / entrySize;

    for (std::size_t i = 0; i < numEntries; ++i) {
      const std::uint8_t *p = d.data() + i * entrySize;
      std::uint8_t rcnm = p[0];
      std::uint32_t rcid = readU32LE(p + 1);
      std::uint64_t key = (static_cast<std::uint64_t>(rcnm) << 32) | rcid;

      auto it = vectors.find(key);
      if (it != vectors.end()) {
        for (const auto &c : it->second.coords) {
          area.exteriorRing.push_back(c);
        }
      }
    }
  }
  return area;
}

std::string attributeKeyForCode(std::uint16_t code)
{
  if(const auto mapped = lookupAttributeAcronym(code); !mapped.empty()) {
    return std::string(mapped);
  }

  return "A" + std::to_string(code);
}

void parseAttributeField(
  const iso8211::Field *field,
  chart_data::Feature &feature)
{
  if(field == nullptr || field->data.empty()) {
    return;
  }

  const auto &data = field->data;
  std::size_t pos = 0;
  while(pos + 2 <= data.size() && data[pos] != 0x1E) {
    const auto attributeCode = readU16LE(data.data() + pos);
    pos += 2;

    const std::size_t valueStart = pos;
    while(pos < data.size() && data[pos] != 0x1F && data[pos] != 0x1E) {
      ++pos;
    }

    std::string value(reinterpret_cast<const char *>(data.data() + valueStart), pos - valueStart);
    if(pos < data.size() && data[pos] == 0x1F) {
      ++pos;
    }

    if(value.empty()) {
      continue;
    }

    const auto attributeKey = attributeKeyForCode(attributeCode);
    char *end = nullptr;
    const auto numericValue = std::strtod(value.c_str(), &end);
    if(end == value.c_str() + value.size()) {
      const auto integralValue = static_cast<std::int64_t>(numericValue);
      if(static_cast<double>(integralValue) == numericValue) {
        feature.attributes[attributeKey] = integralValue;
      } else {
        feature.attributes[attributeKey] = numericValue;
      }
      continue;
    }

    feature.attributes[attributeKey] = std::move(value);
  }
}

}// namespace

S57ReadResult S57Reader::read(const std::string &path) const
{
  std::ifstream ifs(path, std::ios::binary | std::ios::ate);
  if (!ifs) {
    return {false, "cannot open file: " + path, {}};
  }

  const auto size = static_cast<std::size_t>(ifs.tellg());
  ifs.seekg(0, std::ios::beg);

  std::vector<std::uint8_t> data(size);
  if (!ifs.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(size))) {
    return {false, "failed to read file: " + path, {}};
  }

  auto fileName = std::filesystem::path(path).stem().string();
  return readFromMemory(data, fileName);
}

S57ReadResult S57Reader::readFromMemory(
  std::span<const std::uint8_t> data,
  const std::string &name) const
{
  S57ReadResult result;

  // Parse ISO 8211.
  auto iso = iso8211::parse(data);
  if (!iso.ok) {
    result.error = "ISO 8211 parse failed: " + iso.error;
    return result;
  }

  auto &mod = iso.module;

  // Find coordinate multiplication factor from DSPM.
  double comf = 1.0e-7;
  for (const auto &rec : mod.dataRecords) {
    if (findField(rec, kDSPM)) {
      comf = parseCoordinateMultiplier(rec);
      break;
    }
  }

  // Parse vector records for geometry lookup.
  auto vectors = parseVectorRecords(mod, comf);

  // Build dataset metadata.
  chart_data::DatasetMeta meta;
  meta.name = name;
  meta.sourceType = chart_view_chart_source_s57;

  // Try to extract scale from DSID.
  for (const auto &rec : mod.dataRecords) {
    const auto *dsidField = findField(rec, kDSID);
    if (dsidField && dsidField->data.size() > 20) {
      // DSID contains scale and other info, but format varies.
      // For now, leave nativeScale at 0 (unknown).
      break;
    }
  }

  // Parse feature records.
  chart_data::Extent extent;
  bool extentInitialized = false;
  std::uint64_t featureIdCounter = 0;

  for (const auto &rec : mod.dataRecords) {
    const auto *fridField = findField(rec, kFRID);
    if (!fridField || fridField->data.empty()) continue;

    chart_data::Feature feat;
    ++featureIdCounter;
    feat.id = featureIdCounter;

    // FRID subfields: RCNM(1) + RCID(4) + PRIM(1) + GRUP(1) + OBJL(2) + RVER(2) + RUIN(1) = 12 bytes
    const auto &fd = fridField->data;
    if (fd.size() < 12) continue;

    // Skip RCNM(1) and RCID(4).
    S57Prim prim = static_cast<S57Prim>(fd[5]);
    feat.classCode = readU16LE(fd.data() + 7);

    if(const auto mappedAcronym =
         lookupObjectClassAcronym(static_cast<std::uint16_t>(feat.classCode));
       !mappedAcronym.empty()) {
      feat.classAcronym = std::string(mappedAcronym);
    } else {
      feat.classAcronym = "OBJ" + std::to_string(feat.classCode);
    }

    parseAttributeField(findField(rec, kATTF), feat);
    parseAttributeField(findField(rec, kNATF), feat);

    // Build geometry from FSPT references.
    const auto *fsptField = findField(rec, kFSPT);
    feat.geometry = buildGeometry(prim, fsptField, vectors);

    // Update extent from geometry coordinates.
    auto updateExtent = [&](double lon, double lat) {
      if (!extentInitialized) {
        extent.minLon = extent.maxLon = lon;
        extent.minLat = extent.maxLat = lat;
        extentInitialized = true;
      } else {
        extent.minLon = std::min(extent.minLon, lon);
        extent.maxLon = std::max(extent.maxLon, lon);
        extent.minLat = std::min(extent.minLat, lat);
        extent.maxLat = std::max(extent.maxLat, lat);
      }
    };

    std::visit([&](const auto &geo) {
      using T = std::decay_t<decltype(geo)>;
      if constexpr (std::is_same_v<T, chart_data::PointGeometry>) {
        if (geo.position.lon != 0 || geo.position.lat != 0)
          updateExtent(geo.position.lon, geo.position.lat);
      } else if constexpr (std::is_same_v<T, chart_data::LineGeometry>) {
        for (const auto &v : geo.vertices)
          updateExtent(v.lon, v.lat);
      } else if constexpr (std::is_same_v<T, chart_data::AreaGeometry>) {
        for (const auto &v : geo.exteriorRing)
          updateExtent(v.lon, v.lat);
      }
    }, feat.geometry);

    result.dataset.addFeature(std::move(feat));
  }

  meta.extent = extent;
  result.dataset.setMeta(std::move(meta));

  result.ok = true;
  return result;
}

}// namespace chart_view::runtime::s57
