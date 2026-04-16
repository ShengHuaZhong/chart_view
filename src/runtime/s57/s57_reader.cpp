#include "s57_reader.hpp"

#include "iso8211.hpp"
#include "s57_semantic_mapping.hpp"

#include <algorithm>
#include <charconv>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <variant>

namespace chart_view::runtime::s57 {

namespace {

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

struct DsidSummary
{
  std::string datasetName;
  std::uint32_t edition{0};
  std::uint32_t update{0};
};

const iso8211::Field *findField(const iso8211::Record &rec, const std::string &tag)
{
  for(const auto &field : rec.fields) {
    if(field.tag == tag) {
      return &field;
    }
  }

  return nullptr;
}

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
  const auto raw = readU32LE(p);
  std::int32_t value{};
  std::memcpy(&value, &raw, sizeof(value));
  return value;
}

std::string extractAsciiSubfield(const std::vector<std::uint8_t> &data, std::size_t &pos)
{
  const auto start = pos;
  while(pos < data.size() && data[pos] != 0x1F && data[pos] != 0x1E) {
    ++pos;
  }

  std::string value(reinterpret_cast<const char *>(data.data() + start), pos - start);
  if(pos < data.size()) {
    ++pos;
  }

  return value;
}

std::uint32_t parseAsciiUnsigned(std::string_view text) noexcept
{
  std::uint32_t value = 0;
  const auto *begin = text.data();
  const auto *end = begin + text.size();
  const auto [ptr, ec] = std::from_chars(begin, end, value);
  if(ec == std::errc() && ptr == end) {
    return value;
  }

  return 0;
}

DsidSummary parseDsidSummary(const iso8211::Record &rec)
{
  DsidSummary summary;
  const auto *dsidField = findField(rec, kDSID);
  if(dsidField == nullptr || dsidField->data.size() <= 7) {
    return summary;
  }

  // DSID starts with 7 bytes of binary identifiers, followed by ASCII
  // subfields such as DSNM, EDTN, UPDN, UADT, and ISDT.
  std::size_t pos = 7;
  summary.datasetName = extractAsciiSubfield(dsidField->data, pos);
  summary.edition = parseAsciiUnsigned(extractAsciiSubfield(dsidField->data, pos));
  summary.update = parseAsciiUnsigned(extractAsciiSubfield(dsidField->data, pos));
  return summary;
}

double parseCoordinateMultiplier(const iso8211::Record &rec)
{
  const auto *dspmField = findField(rec, kDSPM);
  if(dspmField == nullptr || dspmField->data.size() < 20) {
    return 1.0e-7;
  }

  const auto &data = dspmField->data;
  for(std::size_t pos = 8; pos + 4 <= data.size(); ++pos) {
    if(data[pos] == 0x1F || data[pos] == 0x1E) {
      continue;
    }

    const auto candidate = readU32LE(data.data() + pos);
    if(candidate == 10000000u || candidate == 1000000u || candidate == 100000u) {
      return 1.0 / static_cast<double>(candidate);
    }
  }

  return 1.0e-7;
}

S57Primitive toPrimitive(std::uint8_t raw) noexcept
{
  switch(raw) {
  case 1:
    return S57Primitive::kPoint;
  case 2:
    return S57Primitive::kLine;
  case 3:
    return S57Primitive::kArea;
  default:
    return S57Primitive::kNone;
  }
}

std::string attributeKeyForCode(std::uint16_t code)
{
  if(const auto mapped = lookupAttributeAcronym(code); !mapped.empty()) {
    return std::string(mapped);
  }

  return "A" + std::to_string(code);
}

chart_data::AttributeValue decodeAttributeValue(std::string value)
{
  char *end = nullptr;
  const auto numericValue = std::strtod(value.c_str(), &end);
  if(end == value.c_str() + value.size()) {
    const auto integralValue = static_cast<std::int64_t>(numericValue);
    if(static_cast<double>(integralValue) == numericValue) {
      return integralValue;
    }
    return numericValue;
  }

  return value;
}

void parseAttributeField(
  const iso8211::Field *field,
  S57AttributeMap &attributes)
{
  if(field == nullptr || field->data.empty()) {
    return;
  }

  const auto &data = field->data;
  std::size_t pos = 0;
  while(pos + 2 <= data.size() && data[pos] != 0x1E) {
    const auto attributeCode = readU16LE(data.data() + pos);
    pos += 2;

    const auto valueStart = pos;
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

    attributes.insert_or_assign(attributeKeyForCode(attributeCode), decodeAttributeValue(std::move(value)));
  }
}

std::optional<S57FeatureIdentity> parseFeatureIdentity(const iso8211::Field *field)
{
  if(field == nullptr || field->data.size() < 8) {
    return std::nullopt;
  }

  S57FeatureIdentity identity;
  identity.agency = readU16LE(field->data.data());
  identity.featureId = readU32LE(field->data.data() + 2);
  identity.featureSubdivision = readU16LE(field->data.data() + 6);
  if(!identity.isValid()) {
    return std::nullopt;
  }

  return identity;
}

std::vector<S57SpatialPointer> parseSpatialPointers(const iso8211::Field *field)
{
  std::vector<S57SpatialPointer> pointers;
  if(field == nullptr || field->data.size() < 8) {
    return pointers;
  }

  constexpr std::size_t kEntrySize = 8;
  const auto entryCount = field->data.size() / kEntrySize;
  pointers.reserve(entryCount);
  for(std::size_t i = 0; i < entryCount; ++i) {
    const auto *entry = field->data.data() + i * kEntrySize;
    S57SpatialPointer pointer;
    pointer.recordName = entry[0];
    pointer.recordId = readU32LE(entry + 1);
    pointer.orientation = entry[5];
    pointer.usage = entry[6];
    pointer.mask = entry[7];
    pointers.push_back(pointer);
  }

  return pointers;
}

std::unordered_map<std::uint64_t, S57SourceVectorRecord> parseVectorRecords(
  const iso8211::Module &module,
  double coordinateMultiplier)
{
  std::unordered_map<std::uint64_t, S57SourceVectorRecord> vectors;

  for(const auto &rec : module.dataRecords) {
    const auto *vridField = findField(rec, kVRID);
    if(vridField == nullptr || vridField->data.empty()) {
      continue;
    }

    S57SourceVectorRecord record;
    record.recordName = vridField->data[0];
    if(vridField->data.size() >= 5) {
      record.recordId = readU32LE(vridField->data.data() + 1);
    }
    if(vridField->data.size() >= 7) {
      record.recordVersion = readU16LE(vridField->data.data() + 5);
    }
    if(vridField->data.size() >= 8) {
      record.updateInstruction = vridField->data[7];
    }

    const auto *sg2dField = findField(rec, kSG2D);
    if(sg2dField != nullptr && sg2dField->data.size() >= 8) {
      constexpr std::size_t kCoordSize = 8;
      const auto coordCount = sg2dField->data.size() / kCoordSize;
      record.coords.reserve(coordCount);
      for(std::size_t i = 0; i < coordCount; ++i) {
        const auto *coord = sg2dField->data.data() + i * kCoordSize;
        const auto lat = static_cast<double>(readI32LE(coord)) * coordinateMultiplier;
        const auto lon = static_cast<double>(readI32LE(coord + 4)) * coordinateMultiplier;
        record.coords.push_back({lon, lat});
      }
    }

    const auto *sg3dField = findField(rec, kSG3D);
    if(sg3dField != nullptr && sg3dField->data.size() >= 12) {
      constexpr std::size_t kCoordSize = 12;
      const auto coordCount = sg3dField->data.size() / kCoordSize;
      record.coords.reserve(std::max(record.coords.size(), coordCount));
      for(std::size_t i = 0; i < coordCount; ++i) {
        const auto *coord = sg3dField->data.data() + i * kCoordSize;
        const auto lat = static_cast<double>(readI32LE(coord)) * coordinateMultiplier;
        const auto lon = static_cast<double>(readI32LE(coord + 4)) * coordinateMultiplier;
        record.coords.push_back({lon, lat});
      }
    }

    const auto key = (static_cast<std::uint64_t>(record.recordName) << 32) | record.recordId;
    vectors.insert_or_assign(key, std::move(record));
  }

  return vectors;
}

chart_data::Geometry buildGeometry(
  S57Primitive primitive,
  const std::vector<S57SpatialPointer> &spatialPointers,
  const std::unordered_map<std::uint64_t, S57SourceVectorRecord> &vectors)
{
  if(primitive == S57Primitive::kPoint) {
    if(!spatialPointers.empty()) {
      const auto key =
        (static_cast<std::uint64_t>(spatialPointers.front().recordName) << 32) |
        spatialPointers.front().recordId;
      if(const auto it = vectors.find(key); it != vectors.end() && !it->second.coords.empty()) {
        return chart_data::PointGeometry{it->second.coords.front()};
      }
    }

    return chart_data::PointGeometry{};
  }

  if(primitive == S57Primitive::kLine) {
    chart_data::LineGeometry line;
    for(const auto &pointer : spatialPointers) {
      const auto key = (static_cast<std::uint64_t>(pointer.recordName) << 32) | pointer.recordId;
      if(const auto it = vectors.find(key); it != vectors.end()) {
        line.vertices.insert(line.vertices.end(), it->second.coords.begin(), it->second.coords.end());
      }
    }
    return line;
  }

  chart_data::AreaGeometry area;
  for(const auto &pointer : spatialPointers) {
    const auto key = (static_cast<std::uint64_t>(pointer.recordName) << 32) | pointer.recordId;
    if(const auto it = vectors.find(key); it != vectors.end()) {
      area.exteriorRing.insert(
        area.exteriorRing.end(),
        it->second.coords.begin(),
        it->second.coords.end());
    }
  }

  return area;
}

void updateExtentFromGeometry(
  const chart_data::Geometry &geometry,
  chart_data::Extent &extent,
  bool &extentInitialized)
{
  const auto updateExtent = [&](double lon, double lat) {
    if(!extentInitialized) {
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

  std::visit(
    [&](const auto &geo) {
      using GeometryT = std::decay_t<decltype(geo)>;
      if constexpr (std::is_same_v<GeometryT, chart_data::PointGeometry>) {
        if(geo.position.lon != 0.0 || geo.position.lat != 0.0) {
          updateExtent(geo.position.lon, geo.position.lat);
        }
      } else if constexpr (std::is_same_v<GeometryT, chart_data::LineGeometry>) {
        for(const auto &vertex : geo.vertices) {
          updateExtent(vertex.lon, vertex.lat);
        }
      } else if constexpr (std::is_same_v<GeometryT, chart_data::AreaGeometry>) {
        for(const auto &vertex : geo.exteriorRing) {
          updateExtent(vertex.lon, vertex.lat);
        }
      }
    },
    geometry);
}

std::uint64_t makeDatasetFeatureId(const S57SourceFeature &feature, std::uint64_t fallbackId) noexcept
{
  if(feature.identity.has_value() && feature.identity->isValid()) {
    return (static_cast<std::uint64_t>(feature.identity->agency) << 48) |
           (static_cast<std::uint64_t>(feature.identity->featureId) << 16) |
           feature.identity->featureSubdivision;
  }

  if(feature.recordId != 0) {
    return feature.recordId;
  }

  return fallbackId;
}

chart_data::Feature makeDatasetFeature(const S57SourceFeature &sourceFeature, std::uint64_t fallbackId)
{
  chart_data::Feature feature;
  feature.id = makeDatasetFeatureId(sourceFeature, fallbackId);
  feature.classCode = sourceFeature.classCode;
  feature.classAcronym = sourceFeature.classAcronym;
  feature.geometry = sourceFeature.geometry;

  for(const auto &[key, value] : sourceFeature.attributes) {
    feature.attributes.insert_or_assign(key, value);
  }
  for(const auto &[key, value] : sourceFeature.nationalAttributes) {
    feature.attributes.insert_or_assign(key, value);
  }

  return feature;
}

S57ReadResult parseS57Data(
  std::span<const std::uint8_t> data,
  const std::string &datasetName,
  const std::string &sourceName,
  const std::string &sourcePath)
{
  S57ReadResult result;

  const auto isoResult = iso8211::parse(data);
  if(!isoResult.ok) {
    result.error = "ISO 8211 parse failed: " + isoResult.error;
    return result;
  }

  const auto &module = isoResult.module;
  auto *sourceModel = &result.sourceModel;
  sourceModel->sourcePath = sourcePath;
  sourceModel->sourceName = sourceName;

  double coordinateMultiplier = 1.0e-7;
  DsidSummary dsidSummary;
  bool haveCoordinateMultiplier = false;
  bool haveDsid = false;
  for(const auto &rec : module.dataRecords) {
    if(!haveCoordinateMultiplier && findField(rec, kDSPM) != nullptr) {
      coordinateMultiplier = parseCoordinateMultiplier(rec);
      haveCoordinateMultiplier = true;
    }
    if(!haveDsid && findField(rec, kDSID) != nullptr) {
      dsidSummary = parseDsidSummary(rec);
      haveDsid = true;
    }
    if(haveCoordinateMultiplier && haveDsid) {
      break;
    }
  }
  sourceModel->coordinateMultiplier = coordinateMultiplier;
  sourceModel->declaredDatasetName = dsidSummary.datasetName;
  sourceModel->vectors = parseVectorRecords(module, coordinateMultiplier);

  chart_data::DatasetMeta meta;
  meta.name = !datasetName.empty() ? datasetName :
              (!dsidSummary.datasetName.empty() ? dsidSummary.datasetName : sourceName);
  meta.sourceType = chart_view_chart_source_s57;
  meta.edition = dsidSummary.edition;
  meta.update = dsidSummary.update;

  chart_data::Extent extent;
  bool extentInitialized = false;
  sourceModel->features.reserve(module.dataRecords.size());

  for(const auto &rec : module.dataRecords) {
    const auto *fridField = findField(rec, kFRID);
    if(fridField == nullptr || fridField->data.size() < 12) {
      continue;
    }

    const auto &frid = fridField->data;
    S57SourceFeature sourceFeature;
    sourceFeature.recordId = readU32LE(frid.data() + 1);
    sourceFeature.primitive = toPrimitive(frid[5]);
    sourceFeature.classCode = readU16LE(frid.data() + 7);
    sourceFeature.recordVersion = readU16LE(frid.data() + 9);
    sourceFeature.updateInstruction = frid[11];

    if(const auto mapped = lookupObjectClassAcronym(static_cast<std::uint16_t>(sourceFeature.classCode));
       !mapped.empty()) {
      sourceFeature.classAcronym = std::string(mapped);
    } else {
      sourceFeature.classAcronym = "OBJ" + std::to_string(sourceFeature.classCode);
    }

    sourceFeature.identity = parseFeatureIdentity(findField(rec, kFOID));
    sourceFeature.spatialPointers = parseSpatialPointers(findField(rec, kFSPT));
    parseAttributeField(findField(rec, kATTF), sourceFeature.attributes);
    parseAttributeField(findField(rec, kNATF), sourceFeature.nationalAttributes);
    sourceFeature.geometry =
      buildGeometry(sourceFeature.primitive, sourceFeature.spatialPointers, sourceModel->vectors);
    updateExtentFromGeometry(sourceFeature.geometry, extent, extentInitialized);

    sourceModel->features.push_back(std::move(sourceFeature));
  }

  if(extentInitialized) {
    meta.extent = extent;
  }
  sourceModel->datasetMeta = meta;

  const auto effectiveSourceName = !sourceName.empty() ? sourceName : meta.name;
  if(!sourcePath.empty()) {
    sourceModel->sourceManifest =
      buildSourceManifestForPath(sourcePath, effectiveSourceName, meta.edition, meta.update);
    sourceModel->updateManifest =
      buildUpdateManifestForBasePath(sourcePath, meta.edition, meta.update);
  } else {
    sourceModel->sourceManifest =
      buildSourceManifestForBuffer(effectiveSourceName, data, meta.edition, meta.update);
    sourceModel->updateManifest.baseName = effectiveSourceName;
    sourceModel->updateManifest.edition = meta.edition;
    sourceModel->updateManifest.baseUpdate = meta.update;
    sourceModel->updateManifest.highestContiguousUpdate = meta.update;
    sourceModel->updateManifest.nextMissingUpdate = meta.update + 1u;
  }

  result.dataset.setMeta(meta);
  result.dataset.reserveFeatures(sourceModel->features.size());
  std::uint64_t fallbackId = 0;
  for(const auto &sourceFeature : sourceModel->features) {
    result.dataset.addFeature(makeDatasetFeature(sourceFeature, ++fallbackId));
  }

  result.ok = true;
  return result;
}

}// namespace

S57ReadResult S57Reader::read(const std::string &path) const
{
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if(!stream) {
    S57ReadResult result;
    result.error = "cannot open file: " + path;
    return result;
  }

  const auto size = static_cast<std::size_t>(stream.tellg());
  stream.seekg(0, std::ios::beg);

  std::vector<std::uint8_t> data(size);
  if(!stream.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(size))) {
    S57ReadResult result;
    result.error = "failed to read file: " + path;
    return result;
  }

  const auto filePath = std::filesystem::path(path);
  return parseS57Data(data, filePath.stem().string(), filePath.filename().string(), path);
}

S57ReadResult S57Reader::readFromMemory(
  std::span<const std::uint8_t> data,
  const std::string &name) const
{
  return parseS57Data(data, name, name, {});
}

}// namespace chart_view::runtime::s57
