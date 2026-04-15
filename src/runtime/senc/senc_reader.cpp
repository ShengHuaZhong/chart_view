#include "senc_reader.hpp"

#include <cstring>
#include <fstream>

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

  err = decodeSections(descs, blob, result.dataset, result.manifest);
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

std::string SencReader::validateHeader(const FileHeader &fh, std::size_t blobSize) const
{
  if (fh.magic != kSencMagic)
    return "invalid magic";
  if (fh.formatVersion != kSencFormatVersion)
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
  const std::vector<SectionDesc> &descs,
  std::span<const std::uint8_t> blob,
  chart_data::FeatureChartDataset &out,
  std::optional<SourceManifest> &manifestOut) const
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
    // Stub sections -- ignore for Phase 1.
    case SectionType::kSpatialIndex:
    case SectionType::kRenderCache:
    case SectionType::kPickIndex:
    case SectionType::kStringTable:
    case SectionType::kCount_:
      break;
    }

    if (!err.empty())
      return err;
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
      if (typeTag == 0) {
        std::int64_t v = 0;
        if (!readRaw(payload, offset, v))
          return "AttributeBlob: cannot read int64 value";
        val = v;
      } else if (typeTag == 1) {
        double v = 0.0;
        if (!readRaw(payload, offset, v))
          return "AttributeBlob: cannot read double value";
        val = v;
      } else if (typeTag == 2) {
        std::string v;
        if (!readString(payload, offset, v))
          return "AttributeBlob: cannot read string value";
        val = std::move(v);
      } else {
        return "AttributeBlob: unknown type tag " + std::to_string(typeTag);
      }

      feat.attributes[key] = std::move(val);
    }
  }

  return {};
}

}// namespace chart_view::runtime::senc
