#include "senc_reader.hpp"

#include <cstring>
#include <fstream>
#include <unordered_map>

namespace chart_view::runtime::senc {

namespace {

// CRC-32 matching the writer.
std::uint32_t crc32(const std::uint8_t *data, std::size_t size) noexcept
{
  std::uint32_t crc = 0xFFFFFFFFu;
  for (std::size_t i = 0; i < size; ++i) {
    crc ^= data[i];
    for (int j = 0; j < 8; ++j) {
      if (crc & 1u)
        crc = (crc >> 1u) ^ 0xEDB88320u;
      else
        crc >>= 1u;
    }
  }
  return crc ^ 0xFFFFFFFFu;
}

// Safe read helpers -- return false if out of bounds.
template<typename T>
bool readRaw(std::span<const std::uint8_t> buf, std::size_t &offset, T &out)
{
  static_assert(std::is_trivially_copyable_v<T>);
  if (offset + sizeof(T) > buf.size()) return false;
  std::memcpy(&out, buf.data() + offset, sizeof(T));
  offset += sizeof(T);
  return true;
}

bool readString(std::span<const std::uint8_t> buf, std::size_t &offset, std::string &out)
{
  std::uint32_t len = 0;
  if (!readRaw(buf, offset, len)) return false;
  if (offset + len > buf.size()) return false;
  out.assign(reinterpret_cast<const char *>(buf.data() + offset), len);
  offset += len;
  return true;
}

bool readAttributeValue(
  std::span<const std::uint8_t> buf,
  std::size_t &offset,
  chart_view::runtime::chart_data::AttributeValue &out)
{
  std::uint8_t typeTag = 0;
  if(!readRaw(buf, offset, typeTag)) return false;

  if(typeTag == 0) {
    std::int64_t value = 0;
    if(!readRaw(buf, offset, value)) return false;
    out = value;
    return true;
  }

  if(typeTag == 1) {
    double value = 0.0;
    if(!readRaw(buf, offset, value)) return false;
    out = value;
    return true;
  }

  if(typeTag == 2) {
    std::string value;
    if(!readString(buf, offset, value)) return false;
    out = std::move(value);
    return true;
  }

  if(typeTag == 3) {
    std::uint32_t count = 0;
    if(!readRaw(buf, offset, count)) return false;
    chart_view::runtime::chart_data::AttributeIntList values(count);
    for(std::uint32_t i = 0; i < count; ++i) {
      if(!readRaw(buf, offset, values[i])) return false;
    }
    out = std::move(values);
    return true;
  }

  if(typeTag == 4) {
    std::uint32_t count = 0;
    if(!readRaw(buf, offset, count)) return false;
    chart_view::runtime::chart_data::AttributeDoubleList values(count);
    for(std::uint32_t i = 0; i < count; ++i) {
      if(!readRaw(buf, offset, values[i])) return false;
    }
    out = std::move(values);
    return true;
  }

  if(typeTag == 5) {
    std::uint32_t count = 0;
    if(!readRaw(buf, offset, count)) return false;
    chart_view::runtime::chart_data::AttributeStringList values;
    values.reserve(count);
    for(std::uint32_t i = 0; i < count; ++i) {
      std::string value;
      if(!readString(buf, offset, value)) return false;
      values.push_back(std::move(value));
    }
    out = std::move(values);
    return true;
  }

  return false;
}

bool readAttributeMap(
  std::span<const std::uint8_t> buf,
  std::size_t &offset,
  chart_view::runtime::s57::S57AttributeMap &out)
{
  std::uint32_t count = 0;
  if(!readRaw(buf, offset, count)) return false;

  for(std::uint32_t i = 0; i < count; ++i) {
    std::string key;
    if(!readString(buf, offset, key)) return false;
    chart_view::runtime::chart_data::AttributeValue value;
    if(!readAttributeValue(buf, offset, value)) return false;
    out.insert_or_assign(std::move(key), std::move(value));
  }

  return true;
}

bool readGeometry(
  std::span<const std::uint8_t> buf,
  std::size_t &offset,
  chart_view::runtime::chart_data::Geometry &out)
{
  std::uint8_t geometryType = 0;
  if(!readRaw(buf, offset, geometryType)) return false;

  if(static_cast<chart_view::runtime::chart_data::GeometryType>(geometryType) ==
     chart_view::runtime::chart_data::GeometryType::kPoint) {
    chart_view::runtime::chart_data::PointGeometry point;
    if(!readRaw(buf, offset, point.position.lon)) return false;
    if(!readRaw(buf, offset, point.position.lat)) return false;
    out = point;
    return true;
  }

  if(static_cast<chart_view::runtime::chart_data::GeometryType>(geometryType) ==
     chart_view::runtime::chart_data::GeometryType::kLine) {
    chart_view::runtime::chart_data::LineGeometry line;
    std::uint32_t vertexCount = 0;
    if(!readRaw(buf, offset, vertexCount)) return false;
    line.vertices.resize(vertexCount);
    for(std::uint32_t i = 0; i < vertexCount; ++i) {
      if(!readRaw(buf, offset, line.vertices[i].lon)) return false;
      if(!readRaw(buf, offset, line.vertices[i].lat)) return false;
    }
    out = std::move(line);
    return true;
  }

  chart_view::runtime::chart_data::AreaGeometry area;
  std::uint32_t exteriorCount = 0;
  if(!readRaw(buf, offset, exteriorCount)) return false;
  area.exteriorRing.resize(exteriorCount);
  for(std::uint32_t i = 0; i < exteriorCount; ++i) {
    if(!readRaw(buf, offset, area.exteriorRing[i].lon)) return false;
    if(!readRaw(buf, offset, area.exteriorRing[i].lat)) return false;
  }

  std::uint32_t holeCount = 0;
  if(!readRaw(buf, offset, holeCount)) return false;
  area.interiorRings.resize(holeCount);
  for(std::uint32_t holeIndex = 0; holeIndex < holeCount; ++holeIndex) {
    std::uint32_t ringCount = 0;
    if(!readRaw(buf, offset, ringCount)) return false;
    area.interiorRings[holeIndex].resize(ringCount);
    for(std::uint32_t vertexIndex = 0; vertexIndex < ringCount; ++vertexIndex) {
      if(!readRaw(buf, offset, area.interiorRings[holeIndex][vertexIndex].lon)) return false;
      if(!readRaw(buf, offset, area.interiorRings[holeIndex][vertexIndex].lat)) return false;
    }
  }

  out = std::move(area);
  return true;
}

}// namespace

SencReadResult SencReader::read(std::span<const std::uint8_t> blob) const
{
  SencReadResult result;

  if (blob.size() < sizeof(FileHeader)) {
    result.error = "blob too small for file header";
    return result;
  }

  // Read header.
  FileHeader fh{};
  std::memcpy(&fh, blob.data(), sizeof(FileHeader));

  auto err = validateHeader(fh, blob.size());
  if (!err.empty()) {
    result.error = std::move(err);
    return result;
  }

  // Read section table.
  const std::size_t tableOffset = sizeof(FileHeader);
  const std::size_t tableEnd = tableOffset + fh.sectionCount * sizeof(SectionDesc);
  if (tableEnd > blob.size()) {
    result.error = "blob too small for section table";
    return result;
  }

  std::vector<SectionDesc> descs(fh.sectionCount);
  std::memcpy(descs.data(), blob.data() + tableOffset, fh.sectionCount * sizeof(SectionDesc));

  err = validateSectionTable(descs, fh, blob);
  if (!err.empty()) {
    result.error = std::move(err);
    return result;
  }

  err = decodeSections(fh, descs, blob, result.dataset, result.manifest, result.sourceModel);
  if (!err.empty()) {
    result.error = std::move(err);
    return result;
  }

  result.ok = true;
  return result;
}

SencReadResult SencReader::readFromFile(const std::string &path) const
{
  std::ifstream ifs(path, std::ios::binary | std::ios::ate);
  if (!ifs) {
    SencReadResult r;
    r.error = "cannot open file: " + path;
    return r;
  }

  const auto size = static_cast<std::size_t>(ifs.tellg());
  ifs.seekg(0, std::ios::beg);

  std::vector<std::uint8_t> blob(size);
  if (!ifs.read(reinterpret_cast<char *>(blob.data()), static_cast<std::streamsize>(size))) {
    SencReadResult r;
    r.error = "failed to read file: " + path;
    return r;
  }

  return read(blob);
}

SencCatalogMetaReadResult SencReader::readCatalogMeta(std::span<const std::uint8_t> blob) const
{
  SencCatalogMetaReadResult result;

  if(blob.size() < sizeof(FileHeader)) {
    result.error = "blob too small for file header";
    return result;
  }

  FileHeader fh{};
  std::memcpy(&fh, blob.data(), sizeof(FileHeader));

  auto err = validateHeader(fh, blob.size());
  if(!err.empty()) {
    result.error = std::move(err);
    return result;
  }

  const std::size_t tableOffset = sizeof(FileHeader);
  const std::size_t tableEnd = tableOffset + fh.sectionCount * sizeof(SectionDesc);
  if(tableEnd > blob.size()) {
    result.error = "blob too small for section table";
    return result;
  }

  std::vector<SectionDesc> descs(fh.sectionCount);
  std::memcpy(descs.data(), blob.data() + tableOffset, fh.sectionCount * sizeof(SectionDesc));

  err = validateSectionTable(descs, fh, blob);
  if(!err.empty()) {
    result.error = std::move(err);
    return result;
  }

  err = decodeCatalogSections(descs, blob, result.meta, result.manifest);
  if(!err.empty()) {
    result.error = std::move(err);
    return result;
  }

  result.ok = true;
  return result;
}

SencCatalogMetaReadResult SencReader::readCatalogMetaFromFile(const std::string &path) const
{
  std::ifstream ifs(path, std::ios::binary | std::ios::ate);
  if(!ifs) {
    SencCatalogMetaReadResult result;
    result.error = "cannot open file: " + path;
    return result;
  }

  const auto size = static_cast<std::size_t>(ifs.tellg());
  ifs.seekg(0, std::ios::beg);

  std::vector<std::uint8_t> blob(size);
  if(!ifs.read(reinterpret_cast<char *>(blob.data()), static_cast<std::streamsize>(size))) {
    SencCatalogMetaReadResult result;
    result.error = "failed to read file: " + path;
    return result;
  }

  return readCatalogMeta(blob);
}

std::string SencReader::validateHeader(const FileHeader &fh, std::size_t blobSize) const
{
  if (fh.magic != kSencMagic)
    return "invalid magic";
  if (fh.formatVersion != kSencFormatVersionV1 && fh.formatVersion != kSencFormatVersionV2)
    return "unsupported format version";
  if (fh.totalFileSize != static_cast<std::uint32_t>(blobSize))
    return "totalFileSize mismatch";
  if (fh.sectionCount == 0)
    return "zero section count";
  return {};
}

std::string SencReader::validateSectionTable(
  const std::vector<SectionDesc> &descs,
  const FileHeader &fh,
  std::span<const std::uint8_t> blob) const
{
  for (std::uint32_t i = 0; i < descs.size(); ++i) {
    const auto &desc = descs[i];

    if (static_cast<std::uint16_t>(desc.type) != i)
      return "section type mismatch at index " + std::to_string(i);

    if (desc.offset + desc.size > fh.totalFileSize)
      return "section overflow at index " + std::to_string(i);

    // Verify CRC-32 for non-empty sections.
    if (desc.size > 0) {
      auto computed = crc32(blob.data() + desc.offset, desc.size);
      if (computed != desc.checksum)
        return "CRC mismatch at section " + std::to_string(i);
    }
  }

  return {};
}

std::string SencReader::decodeSections(
  const FileHeader &fh,
  const std::vector<SectionDesc> &descs,
  std::span<const std::uint8_t> blob,
  chart_data::FeatureChartDataset &out,
  std::optional<SourceManifest> &manifestOut,
  std::optional<s57::S57SourceModel> &sourceModelOut) const
{
  for (const auto &desc : descs) {
    auto payload = blob.subspan(desc.offset, desc.size);

    std::string err;
    switch (desc.type) {
    case SectionType::kFileHeader:
      // No payload data.
      break;
    case SectionType::kSourceManifest: {
      SourceManifest m;
      err = decodeSourceManifest(payload, m);
      if (err.empty())
        manifestOut = std::move(m);
      break;
    }
    case SectionType::kDatasetMeta:
      err = decodeDatasetMeta(payload, out);
      break;
    case SectionType::kFeatureTable:
      err = decodeFeatureTable(payload, out);
      break;
    case SectionType::kGeometryBlob:
      err = decodeGeometryBlob(payload, out);
      break;
    case SectionType::kAttributeBlob:
      err = decodeAttributeBlob(payload, out);
      break;
    case SectionType::kSpatialIndex:
      break;
    case SectionType::kRenderCache:
      if(fh.formatVersion == kSencFormatVersionV2 && !payload.empty()) {
        if(!sourceModelOut.has_value()) {
          sourceModelOut.emplace();
        }
        err = decodeS57SemanticManifestV2(payload, *sourceModelOut);
      }
      break;
    case SectionType::kPickIndex:
      if(fh.formatVersion == kSencFormatVersionV2 && !payload.empty()) {
        if(!sourceModelOut.has_value()) {
          sourceModelOut.emplace();
        }
        err = decodeS57FeatureSemanticsV2(payload, *sourceModelOut);
      }
      break;
    case SectionType::kStringTable:
      if(fh.formatVersion == kSencFormatVersionV2 && !payload.empty()) {
        if(!sourceModelOut.has_value()) {
          sourceModelOut.emplace();
        }
        err = decodeS57VectorRecordsV2(payload, *sourceModelOut);
      }
      break;
    case SectionType::kCount_:
      break;
    }

    if (!err.empty())
      return err;
  }

  if(sourceModelOut.has_value()) {
    auto &sourceModel = sourceModelOut.value();
    sourceModel.datasetMeta = out.meta();
    if(manifestOut.has_value()) {
      sourceModel.sourceManifest = manifestOut.value();
    }
    if(sourceModel.sourceName.empty()) {
      sourceModel.sourceName = manifestOut.has_value() ? manifestOut->name : out.meta().name;
    }
    if(sourceModel.declaredDatasetName.empty()) {
      sourceModel.declaredDatasetName = out.meta().name;
    }
    if(sourceModel.updateManifest.baseName.empty()) {
      sourceModel.updateManifest.baseName = sourceModel.sourceName;
    }
  }

  return {};
}

std::string SencReader::decodeCatalogSections(
  const std::vector<SectionDesc> &descs,
  std::span<const std::uint8_t> blob,
  chart_data::DatasetMeta &metaOut,
  std::optional<SourceManifest> &manifestOut) const
{
  bool sawDatasetMeta = false;

  for(const auto &desc : descs) {
    auto payload = blob.subspan(desc.offset, desc.size);

    if(desc.type == SectionType::kSourceManifest) {
      SourceManifest manifest;
      auto err = decodeSourceManifest(payload, manifest);
      if(!err.empty()) {
        return err;
      }
      manifestOut = std::move(manifest);
    } else if(desc.type == SectionType::kDatasetMeta) {
      chart_data::FeatureChartDataset dataset;
      auto err = decodeDatasetMeta(payload, dataset);
      if(!err.empty()) {
        return err;
      }
      metaOut = dataset.meta();
      sawDatasetMeta = true;
    }
  }

  if(!sawDatasetMeta) {
    return "missing DatasetMeta section";
  }

  return {};
}

std::string SencReader::decodeSourceManifest(
  std::span<const std::uint8_t> payload,
  SourceManifest &manifestOut) const
{
  std::size_t offset = 0;

  std::uint32_t srcType = 0;
  if (!readRaw(payload, offset, srcType))
    return "SourceManifest: cannot read sourceType";
  manifestOut.sourceType = static_cast<chart_view_chart_source_type_t>(srcType);

  if (!readString(payload, offset, manifestOut.name))
    return "SourceManifest: cannot read name";

  if (!readRaw(payload, offset, manifestOut.sourceSize))
    return "SourceManifest: cannot read sourceSize";
  if (!readRaw(payload, offset, manifestOut.sourceTimestamp))
    return "SourceManifest: cannot read sourceTimestamp";
  if (!readRaw(payload, offset, manifestOut.sourceHash))
    return "SourceManifest: cannot read sourceHash";
  if (!readRaw(payload, offset, manifestOut.edition))
    return "SourceManifest: cannot read edition";
  if (!readRaw(payload, offset, manifestOut.update))
    return "SourceManifest: cannot read update";

  return {};
}

std::string SencReader::decodeDatasetMeta(
  std::span<const std::uint8_t> payload,
  chart_data::FeatureChartDataset &out) const
{
  std::size_t offset = 0;
  chart_data::DatasetMeta meta;

  if (!readString(payload, offset, meta.name))
    return "DatasetMeta: cannot read name";

  std::uint32_t srcType = 0;
  if (!readRaw(payload, offset, srcType))
    return "DatasetMeta: cannot read sourceType";
  meta.sourceType = static_cast<chart_view_chart_source_type_t>(srcType);

  if (!readRaw(payload, offset, meta.nativeScale))
    return "DatasetMeta: cannot read nativeScale";
  if (!readRaw(payload, offset, meta.extent.minLon))
    return "DatasetMeta: cannot read minLon";
  if (!readRaw(payload, offset, meta.extent.minLat))
    return "DatasetMeta: cannot read minLat";
  if (!readRaw(payload, offset, meta.extent.maxLon))
    return "DatasetMeta: cannot read maxLon";
  if (!readRaw(payload, offset, meta.extent.maxLat))
    return "DatasetMeta: cannot read maxLat";
  if (!readRaw(payload, offset, meta.usageBand))
    return "DatasetMeta: cannot read usageBand";
  if (!readRaw(payload, offset, meta.edition))
    return "DatasetMeta: cannot read edition";
  if (!readRaw(payload, offset, meta.update))
    return "DatasetMeta: cannot read update";

  out.setMeta(std::move(meta));
  return {};
}

std::string SencReader::decodeFeatureTable(
  std::span<const std::uint8_t> payload,
  chart_data::FeatureChartDataset &out) const
{
  std::size_t offset = 0;

  std::uint32_t featureCount = 0;
  if (!readRaw(payload, offset, featureCount))
    return "FeatureTable: cannot read featureCount";

  out.reserveFeatures(featureCount);

  for (std::uint32_t i = 0; i < featureCount; ++i) {
    chart_data::Feature feat;

    if (!readRaw(payload, offset, feat.id))
      return "FeatureTable: cannot read feature id at " + std::to_string(i);
    if (!readRaw(payload, offset, feat.classCode))
      return "FeatureTable: cannot read classCode at " + std::to_string(i);
    if (!readString(payload, offset, feat.classAcronym))
      return "FeatureTable: cannot read classAcronym at " + std::to_string(i);

    std::uint8_t geoType = 0;
    if (!readRaw(payload, offset, geoType))
      return "FeatureTable: cannot read geoType at " + std::to_string(i);

    // Set placeholder geometry based on type -- actual geometry comes from GeometryBlob.
    switch (static_cast<chart_data::GeometryType>(geoType)) {
    case chart_data::GeometryType::kPoint:
      feat.geometry = chart_data::PointGeometry{};
      break;
    case chart_data::GeometryType::kLine:
      feat.geometry = chart_data::LineGeometry{};
      break;
    case chart_data::GeometryType::kArea:
      feat.geometry = chart_data::AreaGeometry{};
      break;
    }

    std::uint32_t attrCount = 0;
    if (!readRaw(payload, offset, attrCount))
      return "FeatureTable: cannot read attrCount at " + std::to_string(i);

    // Read attribute key names (values come from AttributeBlob).
    // Store empty placeholders.
    for (std::uint32_t a = 0; a < attrCount; ++a) {
      std::string key;
      if (!readString(payload, offset, key))
        return "FeatureTable: cannot read attr key at " + std::to_string(i);
      feat.attributes[key] = std::int64_t{0}; // placeholder
    }

    out.addFeature(std::move(feat));
  }

  return {};
}

std::string SencReader::decodeGeometryBlob(
  std::span<const std::uint8_t> payload,
  chart_data::FeatureChartDataset &out) const
{
  std::size_t offset = 0;

  auto &features = out.features();

  for (std::size_t i = 0; i < features.size(); ++i) {
    std::uint8_t geoType = 0;
    if (!readRaw(payload, offset, geoType))
      return "GeometryBlob: cannot read geoType at " + std::to_string(i);

    switch (static_cast<chart_data::GeometryType>(geoType)) {
    case chart_data::GeometryType::kPoint: {
      chart_data::PointGeometry pt;
      if (!readRaw(payload, offset, pt.position.lon))
        return "GeometryBlob: cannot read point lon at " + std::to_string(i);
      if (!readRaw(payload, offset, pt.position.lat))
        return "GeometryBlob: cannot read point lat at " + std::to_string(i);
      features[i].geometry = pt;
      break;
    }
    case chart_data::GeometryType::kLine: {
      chart_data::LineGeometry line;
      std::uint32_t vertCount = 0;
      if (!readRaw(payload, offset, vertCount))
        return "GeometryBlob: cannot read line vertCount at " + std::to_string(i);
      line.vertices.resize(vertCount);
      for (std::uint32_t v = 0; v < vertCount; ++v) {
        if (!readRaw(payload, offset, line.vertices[v].lon))
          return "GeometryBlob: cannot read line vertex lon";
        if (!readRaw(payload, offset, line.vertices[v].lat))
          return "GeometryBlob: cannot read line vertex lat";
      }
      features[i].geometry = std::move(line);
      break;
    }
    case chart_data::GeometryType::kArea: {
      chart_data::AreaGeometry area;
      std::uint32_t extCount = 0;
      if (!readRaw(payload, offset, extCount))
        return "GeometryBlob: cannot read area extCount at " + std::to_string(i);
      area.exteriorRing.resize(extCount);
      for (std::uint32_t v = 0; v < extCount; ++v) {
        if (!readRaw(payload, offset, area.exteriorRing[v].lon))
          return "GeometryBlob: cannot read area ext vertex lon";
        if (!readRaw(payload, offset, area.exteriorRing[v].lat))
          return "GeometryBlob: cannot read area ext vertex lat";
      }
      std::uint32_t holeCount = 0;
      if (!readRaw(payload, offset, holeCount))
        return "GeometryBlob: cannot read area holeCount at " + std::to_string(i);
      area.interiorRings.resize(holeCount);
      for (std::uint32_t h = 0; h < holeCount; ++h) {
        std::uint32_t ringCount = 0;
        if (!readRaw(payload, offset, ringCount))
          return "GeometryBlob: cannot read ring count";
        area.interiorRings[h].resize(ringCount);
        for (std::uint32_t v = 0; v < ringCount; ++v) {
          if (!readRaw(payload, offset, area.interiorRings[h][v].lon))
            return "GeometryBlob: cannot read ring vertex lon";
          if (!readRaw(payload, offset, area.interiorRings[h][v].lat))
            return "GeometryBlob: cannot read ring vertex lat";
        }
      }
      features[i].geometry = std::move(area);
      break;
    }
    }
  }

  return {};
}

std::string SencReader::decodeAttributeBlob(
  std::span<const std::uint8_t> payload,
  chart_data::FeatureChartDataset &out) const
{
  if (payload.empty()) return {};

  std::size_t offset = 0;
  auto &features = out.features();

  // The AttributeBlob stores all attributes for all features in order.
  // For each feature, iterate over its attribute map (same order as written).
  for (auto &feat : features) {
    // We need to replace placeholder values with actual decoded values.
    // Since unordered_map iteration order may differ from write order,
    // the blob stores key+type+value, so we read by key and update.
    const auto attrCount = feat.attributes.size();
    // Clear and rebuild.
    feat.attributes.clear();

    for (std::size_t a = 0; a < attrCount; ++a) {
      std::string key;
      if (!readString(payload, offset, key))
        return "AttributeBlob: cannot read attr key";

      std::uint8_t typeTag = 0;
      if (!readRaw(payload, offset, typeTag))
        return "AttributeBlob: cannot read type tag";

      chart_data::AttributeValue val;
      offset -= sizeof(typeTag);
      if(!readAttributeValue(payload, offset, val)) {
        return "AttributeBlob: unknown type tag " + std::to_string(typeTag);
      }

      feat.attributes[key] = std::move(val);
    }
  }

  return {};
}

std::string SencReader::decodeS57SemanticManifestV2(
  std::span<const std::uint8_t> payload,
  s57::S57SourceModel &out) const
{
  std::size_t offset = 0;
  if(!readRaw(payload, offset, out.coordinateMultiplier))
    return "S57SemanticManifestV2: cannot read coordinateMultiplier";
  if(!readString(payload, offset, out.sourceName))
    return "S57SemanticManifestV2: cannot read sourceName";
  if(!readString(payload, offset, out.declaredDatasetName))
    return "S57SemanticManifestV2: cannot read declaredDatasetName";
  if(!readString(payload, offset, out.updateManifest.baseName))
    return "S57SemanticManifestV2: cannot read update base name";
  if(!readRaw(payload, offset, out.updateManifest.edition))
    return "S57SemanticManifestV2: cannot read edition";
  if(!readRaw(payload, offset, out.updateManifest.baseUpdate))
    return "S57SemanticManifestV2: cannot read baseUpdate";
  if(!readRaw(payload, offset, out.updateManifest.highestContiguousUpdate))
    return "S57SemanticManifestV2: cannot read highestContiguousUpdate";
  if(!readRaw(payload, offset, out.updateManifest.lastAppliedUpdate))
    return "S57SemanticManifestV2: cannot read lastAppliedUpdate";
  if(!readRaw(payload, offset, out.updateManifest.nextMissingUpdate))
    return "S57SemanticManifestV2: cannot read nextMissingUpdate";

  std::uint32_t availableCount = 0;
  if(!readRaw(payload, offset, availableCount))
    return "S57SemanticManifestV2: cannot read available update count";
  out.updateManifest.availableUpdates.clear();
  out.updateManifest.availableUpdates.reserve(availableCount);
  for(std::uint32_t i = 0; i < availableCount; ++i) {
    s57::S57UpdateFile update;
    if(!readString(payload, offset, update.name))
      return "S57SemanticManifestV2: cannot read update name";
    if(!readRaw(payload, offset, update.updateNumber))
      return "S57SemanticManifestV2: cannot read update number";
    if(!readRaw(payload, offset, update.sourceSize))
      return "S57SemanticManifestV2: cannot read update sourceSize";
    if(!readRaw(payload, offset, update.sourceTimestamp))
      return "S57SemanticManifestV2: cannot read update sourceTimestamp";
    out.updateManifest.availableUpdates.push_back(std::move(update));
  }

  std::uint32_t appliedCount = 0;
  if(!readRaw(payload, offset, appliedCount))
    return "S57SemanticManifestV2: cannot read applied update count";
  out.updateManifest.appliedUpdates.clear();
  out.updateManifest.appliedUpdates.reserve(appliedCount);
  for(std::uint32_t i = 0; i < appliedCount; ++i) {
    std::uint32_t updateNumber = 0;
    if(!readRaw(payload, offset, updateNumber))
      return "S57SemanticManifestV2: cannot read applied update number";
    out.updateManifest.appliedUpdates.push_back(updateNumber);
  }

  return {};
}

std::string SencReader::decodeS57FeatureSemanticsV2(
  std::span<const std::uint8_t> payload,
  s57::S57SourceModel &out) const
{
  std::size_t offset = 0;
  std::uint32_t featureCount = 0;
  if(!readRaw(payload, offset, featureCount))
    return "S57FeatureSemanticsV2: cannot read feature count";

  out.features.clear();
  out.features.reserve(featureCount);
  for(std::uint32_t i = 0; i < featureCount; ++i) {
    std::uint64_t datasetFeatureId = 0;
    if(!readRaw(payload, offset, datasetFeatureId))
      return "S57FeatureSemanticsV2: cannot read dataset feature id";

    s57::S57SourceFeature feature;
    if(!readRaw(payload, offset, feature.recordId))
      return "S57FeatureSemanticsV2: cannot read recordId";
    if(!readRaw(payload, offset, feature.recordVersion))
      return "S57FeatureSemanticsV2: cannot read recordVersion";
    if(!readRaw(payload, offset, feature.updateInstruction))
      return "S57FeatureSemanticsV2: cannot read updateInstruction";

    std::uint8_t primitive = 0;
    if(!readRaw(payload, offset, primitive))
      return "S57FeatureSemanticsV2: cannot read primitive";
    feature.primitive = static_cast<s57::S57Primitive>(primitive);

    if(!readRaw(payload, offset, feature.classCode))
      return "S57FeatureSemanticsV2: cannot read classCode";
    if(!readString(payload, offset, feature.classAcronym))
      return "S57FeatureSemanticsV2: cannot read classAcronym";

    std::uint8_t hasIdentity = 0;
    if(!readRaw(payload, offset, hasIdentity))
      return "S57FeatureSemanticsV2: cannot read identity flag";
    if(hasIdentity != 0) {
      s57::S57FeatureIdentity identity;
      if(!readRaw(payload, offset, identity.agency))
        return "S57FeatureSemanticsV2: cannot read identity agency";
      if(!readRaw(payload, offset, identity.featureId))
        return "S57FeatureSemanticsV2: cannot read identity featureId";
      if(!readRaw(payload, offset, identity.featureSubdivision))
        return "S57FeatureSemanticsV2: cannot read identity featureSubdivision";
      feature.identity = identity;
    }

    std::uint32_t spatialCount = 0;
    if(!readRaw(payload, offset, spatialCount))
      return "S57FeatureSemanticsV2: cannot read spatial pointer count";
    feature.spatialPointers.reserve(spatialCount);
    for(std::uint32_t pointerIndex = 0; pointerIndex < spatialCount; ++pointerIndex) {
      s57::S57SpatialPointer pointer;
      if(!readRaw(payload, offset, pointer.recordName))
        return "S57FeatureSemanticsV2: cannot read pointer recordName";
      if(!readRaw(payload, offset, pointer.recordId))
        return "S57FeatureSemanticsV2: cannot read pointer recordId";
      if(!readRaw(payload, offset, pointer.orientation))
        return "S57FeatureSemanticsV2: cannot read pointer orientation";
      if(!readRaw(payload, offset, pointer.usage))
        return "S57FeatureSemanticsV2: cannot read pointer usage";
      if(!readRaw(payload, offset, pointer.mask))
        return "S57FeatureSemanticsV2: cannot read pointer mask";
      feature.spatialPointers.push_back(pointer);
    }

    if(!readAttributeMap(payload, offset, feature.attributes))
      return "S57FeatureSemanticsV2: cannot read standard attributes";
    if(!readAttributeMap(payload, offset, feature.nationalAttributes))
      return "S57FeatureSemanticsV2: cannot read national attributes";
    if(!readGeometry(payload, offset, feature.geometry))
      return "S57FeatureSemanticsV2: cannot read geometry";

    out.features.push_back(std::move(feature));
  }

  return {};
}

std::string SencReader::decodeS57VectorRecordsV2(
  std::span<const std::uint8_t> payload,
  s57::S57SourceModel &out) const
{
  std::size_t offset = 0;
  std::uint32_t vectorCount = 0;
  if(!readRaw(payload, offset, vectorCount))
    return "S57VectorRecordsV2: cannot read vector count";

  out.vectors.clear();
  for(std::uint32_t i = 0; i < vectorCount; ++i) {
    s57::S57SourceVectorRecord vector;
    if(!readRaw(payload, offset, vector.recordName))
      return "S57VectorRecordsV2: cannot read recordName";
    if(!readRaw(payload, offset, vector.recordId))
      return "S57VectorRecordsV2: cannot read recordId";
    if(!readRaw(payload, offset, vector.recordVersion))
      return "S57VectorRecordsV2: cannot read recordVersion";
    if(!readRaw(payload, offset, vector.updateInstruction))
      return "S57VectorRecordsV2: cannot read updateInstruction";

    std::uint32_t coordCount = 0;
    if(!readRaw(payload, offset, coordCount))
      return "S57VectorRecordsV2: cannot read coord count";
    vector.coords.resize(coordCount);
    for(std::uint32_t coordIndex = 0; coordIndex < coordCount; ++coordIndex) {
      if(!readRaw(payload, offset, vector.coords[coordIndex].lon))
        return "S57VectorRecordsV2: cannot read coord lon";
      if(!readRaw(payload, offset, vector.coords[coordIndex].lat))
        return "S57VectorRecordsV2: cannot read coord lat";
    }

    const auto key =
      (static_cast<std::uint64_t>(vector.recordName) << 32) |
      vector.recordId;
    out.vectors.insert_or_assign(key, std::move(vector));
  }

  return {};
}

}// namespace chart_view::runtime::senc
