#include "senc_writer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <numeric>

namespace chart_view::runtime::senc {

namespace {

// CRC-32 (ISO 3309 / ITU-T V.42) lookup table.
struct Crc32Table
{
  std::uint32_t entries[256]{};

  constexpr Crc32Table()
  {
    for (std::uint32_t i = 0; i < 256; ++i) {
      std::uint32_t crc = i;
      for (int j = 0; j < 8; ++j) {
        if (crc & 1u)
          crc = (crc >> 1u) ^ 0xEDB88320u;
        else
          crc >>= 1u;
      }
      entries[i] = crc;
    }
  }
};

inline constexpr Crc32Table kCrc32Table{};

std::uint32_t crc32(const std::uint8_t *data, std::size_t size) noexcept
{
  std::uint32_t crc = 0xFFFFFFFFu;
  for (std::size_t i = 0; i < size; ++i)
    crc = kCrc32Table.entries[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8u);
  return crc ^ 0xFFFFFFFFu;
}

// Helper: append raw bytes of a trivially copyable value.
template<typename T>
void appendRaw(std::vector<std::uint8_t> &buf, const T &val)
{
  static_assert(std::is_trivially_copyable_v<T>);
  const auto offset = buf.size();
  buf.resize(offset + sizeof(T));
  std::memcpy(buf.data() + offset, &val, sizeof(T));
}

// Helper: append a length-prefixed string (uint32 length, then chars, no null terminator).
void appendString(std::vector<std::uint8_t> &buf, const std::string &s)
{
  const auto len = static_cast<std::uint32_t>(s.size());
  appendRaw(buf, len);
  if (len > 0) {
    const auto offset = buf.size();
    buf.resize(offset + len);
    std::memcpy(buf.data() + offset, s.data(), len);
  }
}

void appendAttributeValue(std::vector<std::uint8_t> &buf, const chart_view::runtime::chart_data::AttributeValue &value)
{
  const auto typeTag = static_cast<std::uint8_t>(value.index());
  appendRaw(buf, typeTag);
  if(typeTag == 0) {
    appendRaw(buf, std::get<std::int64_t>(value));
  } else if(typeTag == 1) {
    appendRaw(buf, std::get<double>(value));
  } else if(typeTag == 2) {
    appendString(buf, std::get<std::string>(value));
  } else if(typeTag == 3) {
    const auto &values = std::get<chart_view::runtime::chart_data::AttributeIntList>(value);
    appendRaw(buf, static_cast<std::uint32_t>(values.size()));
    for(const auto entry : values) {
      appendRaw(buf, entry);
    }
  } else if(typeTag == 4) {
    const auto &values = std::get<chart_view::runtime::chart_data::AttributeDoubleList>(value);
    appendRaw(buf, static_cast<std::uint32_t>(values.size()));
    for(const auto entry : values) {
      appendRaw(buf, entry);
    }
  } else {
    const auto &values = std::get<chart_view::runtime::chart_data::AttributeStringList>(value);
    appendRaw(buf, static_cast<std::uint32_t>(values.size()));
    for(const auto &entry : values) {
      appendString(buf, entry);
    }
  }
}

void appendAttributeMap(
  std::vector<std::uint8_t> &buf,
  const chart_view::runtime::s57::S57AttributeMap &attributes)
{
  appendRaw(buf, static_cast<std::uint32_t>(attributes.size()));
  for(const auto &[key, value] : attributes) {
    appendString(buf, key);
    appendAttributeValue(buf, value);
  }
}

std::uint64_t makeS57DatasetFeatureId(
  const chart_view::runtime::s57::S57SourceFeature &feature,
  std::uint64_t fallbackId) noexcept
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

void appendGeometry(
  std::vector<std::uint8_t> &buf,
  const chart_view::runtime::chart_data::Geometry &geometry)
{
  const auto geometryType = chart_view::runtime::chart_data::geometryType(geometry);
  appendRaw(buf, static_cast<std::uint8_t>(geometryType));

  if(geometryType == chart_view::runtime::chart_data::GeometryType::kPoint) {
    const auto &point = std::get<chart_view::runtime::chart_data::PointGeometry>(geometry);
    appendRaw(buf, point.position.lon);
    appendRaw(buf, point.position.lat);
    return;
  }

  if(geometryType == chart_view::runtime::chart_data::GeometryType::kLine) {
    const auto &line = std::get<chart_view::runtime::chart_data::LineGeometry>(geometry);
    appendRaw(buf, static_cast<std::uint32_t>(line.vertices.size()));
    for(const auto &vertex : line.vertices) {
      appendRaw(buf, vertex.lon);
      appendRaw(buf, vertex.lat);
    }
    return;
  }

  const auto &area = std::get<chart_view::runtime::chart_data::AreaGeometry>(geometry);
  appendRaw(buf, static_cast<std::uint32_t>(area.exteriorRing.size()));
  for(const auto &vertex : area.exteriorRing) {
    appendRaw(buf, vertex.lon);
    appendRaw(buf, vertex.lat);
  }
  appendRaw(buf, static_cast<std::uint32_t>(area.interiorRings.size()));
  for(const auto &ring : area.interiorRings) {
    appendRaw(buf, static_cast<std::uint32_t>(ring.size()));
    for(const auto &vertex : ring) {
      appendRaw(buf, vertex.lon);
      appendRaw(buf, vertex.lat);
    }
  }
}

}// namespace

std::vector<std::uint8_t> SencWriter::write(const chart_data::FeatureChartDataset &dataset) const
{
  const auto sectionCount = static_cast<std::uint32_t>(SectionType::kCount_);
  auto payloads = buildSectionPayloads(dataset);

  // Compute header + table size.
  const std::uint32_t headerSize = sizeof(FileHeader);
  const std::uint32_t tableSize = sectionCount * sizeof(SectionDesc);
  const std::uint32_t prefixSize = headerSize + tableSize;

  // Build section descriptors.
  std::vector<SectionDesc> descs(sectionCount);
  std::uint32_t currentOffset = prefixSize;
  for (std::uint32_t i = 0; i < sectionCount; ++i) {
    auto &desc = descs[i];
    desc.type = static_cast<SectionType>(i);
    desc.reserved = 0;
    desc.offset = currentOffset;
    desc.size = static_cast<std::uint32_t>(payloads[i].size());
    desc.checksum = payloads[i].empty()
                      ? 0u
                      : crc32(payloads[i].data(), payloads[i].size());
    currentOffset += desc.size;
  }

  const std::uint32_t totalFileSize = currentOffset;

  // Build file header.
  FileHeader fh;
  fh.magic = kSencMagic;
  fh.formatVersion = m_formatVersion;
  fh.sectionCount = sectionCount;
  fh.totalFileSize = totalFileSize;

  // Assemble output.
  std::vector<std::uint8_t> output;
  output.reserve(totalFileSize);

  appendRaw(output, fh);

  for (const auto &desc : descs)
    appendRaw(output, desc);

  for (const auto &payload : payloads) {
    if (!payload.empty()) {
      const auto off = output.size();
      output.resize(off + payload.size());
      std::memcpy(output.data() + off, payload.data(), payload.size());
    }
  }

  return output;
}

bool SencWriter::writeToFile(const chart_data::FeatureChartDataset &dataset,
                             const std::string &path) const
{
  auto blob = write(dataset);
  std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
  if (!ofs)
    return false;
  ofs.write(reinterpret_cast<const char *>(blob.data()),
            static_cast<std::streamsize>(blob.size()));
  return ofs.good();
}

std::vector<std::vector<std::uint8_t>> SencWriter::buildSectionPayloads(
  const chart_data::FeatureChartDataset &dataset) const
{
  const auto count = static_cast<std::size_t>(SectionType::kCount_);
  std::vector<std::vector<std::uint8_t>> payloads(count);

  // kFileHeader (index 0): empty payload -- header is separate.
  // kSourceManifest (index 1):
  payloads[static_cast<std::size_t>(SectionType::kSourceManifest)] = encodeSourceManifest(dataset);
  // kDatasetMeta (index 2):
  payloads[static_cast<std::size_t>(SectionType::kDatasetMeta)] = encodeDatasetMeta(dataset);
  // kFeatureTable (index 3):
  payloads[static_cast<std::size_t>(SectionType::kFeatureTable)] = encodeFeatureTable(dataset);
  // kGeometryBlob (index 4):
  payloads[static_cast<std::size_t>(SectionType::kGeometryBlob)] = encodeGeometryBlob(dataset);
  // kAttributeBlob (index 5):
  payloads[static_cast<std::size_t>(SectionType::kAttributeBlob)] = encodeAttributeBlob(dataset);
  // kSpatialIndex  (index 6): stub -- still empty in Phase 5 task 69
  if(shouldWriteV2Semantics()) {
    payloads[static_cast<std::size_t>(SectionType::kRenderCache)] = encodeS57SemanticManifestV2();
    payloads[static_cast<std::size_t>(SectionType::kPickIndex)] = encodeS57FeatureSemanticsV2();
    payloads[static_cast<std::size_t>(SectionType::kStringTable)] = encodeS57VectorRecordsV2();
  }

  return payloads;
}

std::vector<std::uint8_t> SencWriter::encodeSourceManifest(
  const chart_data::FeatureChartDataset &dataset) const
{
  std::vector<std::uint8_t> buf;

  const auto manifest = manifestForWrite(dataset);
  appendRaw(buf, static_cast<std::uint32_t>(manifest.sourceType));
  appendString(buf, manifest.name);
  appendRaw(buf, manifest.sourceSize);
  appendRaw(buf, manifest.sourceTimestamp);
  appendRaw(buf, manifest.sourceHash);
  appendRaw(buf, manifest.edition);
  appendRaw(buf, manifest.update);

  return buf;
}

std::vector<std::uint8_t> SencWriter::encodeDatasetMeta(
  const chart_data::FeatureChartDataset &dataset) const
{
  std::vector<std::uint8_t> buf;
  const auto &meta = dataset.meta();

  appendString(buf, meta.name);
  appendRaw(buf, static_cast<std::uint32_t>(meta.sourceType));
  appendRaw(buf, meta.nativeScale);
  appendRaw(buf, meta.extent.minLon);
  appendRaw(buf, meta.extent.minLat);
  appendRaw(buf, meta.extent.maxLon);
  appendRaw(buf, meta.extent.maxLat);
  appendRaw(buf, meta.usageBand);
  appendRaw(buf, meta.edition);
  appendRaw(buf, meta.update);

  return buf;
}

std::vector<std::uint8_t> SencWriter::encodeFeatureTable(
  const chart_data::FeatureChartDataset &dataset) const
{
  std::vector<std::uint8_t> buf;

  const auto featureCount = static_cast<std::uint32_t>(dataset.featureCount());
  appendRaw(buf, featureCount);

  for (const auto &feat : dataset.features()) {
    appendRaw(buf, feat.id);
    appendRaw(buf, feat.classCode);
    appendString(buf, feat.classAcronym);

    // Geometry type
    const auto geoType = static_cast<std::uint8_t>(chart_data::geometryType(feat.geometry));
    appendRaw(buf, geoType);

    // Attribute count
    const auto attrCount = static_cast<std::uint32_t>(feat.attributes.size());
    appendRaw(buf, attrCount);

    // Attribute names (keys only -- values go in AttributeBlob)
    for (const auto &[key, _] : feat.attributes) {
      appendString(buf, key);
    }
  }

  return buf;
}

std::vector<std::uint8_t> SencWriter::encodeGeometryBlob(
  const chart_data::FeatureChartDataset &dataset) const
{
  std::vector<std::uint8_t> buf;

  for (const auto &feat : dataset.features()) {
    const auto geoType = chart_data::geometryType(feat.geometry);
    appendRaw(buf, static_cast<std::uint8_t>(geoType));

    if (geoType == chart_data::GeometryType::kPoint) {
      const auto &pt = std::get<chart_data::PointGeometry>(feat.geometry);
      appendRaw(buf, pt.position.lon);
      appendRaw(buf, pt.position.lat);
    } else if (geoType == chart_data::GeometryType::kLine) {
      const auto &line = std::get<chart_data::LineGeometry>(feat.geometry);
      const auto vertexCount = static_cast<std::uint32_t>(line.vertices.size());
      appendRaw(buf, vertexCount);
      for (const auto &v : line.vertices) {
        appendRaw(buf, v.lon);
        appendRaw(buf, v.lat);
      }
    } else {
      const auto &area = std::get<chart_data::AreaGeometry>(feat.geometry);
      const auto extCount = static_cast<std::uint32_t>(area.exteriorRing.size());
      appendRaw(buf, extCount);
      for (const auto &v : area.exteriorRing) {
        appendRaw(buf, v.lon);
        appendRaw(buf, v.lat);
      }
      const auto holeCount = static_cast<std::uint32_t>(area.interiorRings.size());
      appendRaw(buf, holeCount);
      for (const auto &ring : area.interiorRings) {
        const auto ringCount = static_cast<std::uint32_t>(ring.size());
        appendRaw(buf, ringCount);
        for (const auto &v : ring) {
          appendRaw(buf, v.lon);
          appendRaw(buf, v.lat);
        }
      }
    }
  }

  return buf;
}

std::vector<std::uint8_t> SencWriter::encodeAttributeBlob(
  const chart_data::FeatureChartDataset &dataset) const
{
  std::vector<std::uint8_t> buf;

  for (const auto &feat : dataset.features()) {
    for (const auto &[key, val] : feat.attributes) {
      appendString(buf, key);

      appendAttributeValue(buf, val);
    }
  }

  return buf;
}

std::vector<std::uint8_t> SencWriter::encodeS57SemanticManifestV2() const
{
  std::vector<std::uint8_t> buf;
  if(!shouldWriteV2Semantics()) {
    return buf;
  }

  const auto &sourceModel = m_s57SourceModel.value();
  appendRaw(buf, sourceModel.coordinateMultiplier);
  appendString(buf, sourceModel.sourceName);
  appendString(buf, sourceModel.declaredDatasetName);
  appendString(buf, sourceModel.updateManifest.baseName);
  appendRaw(buf, sourceModel.updateManifest.edition);
  appendRaw(buf, sourceModel.updateManifest.baseUpdate);
  appendRaw(buf, sourceModel.updateManifest.highestContiguousUpdate);
  appendRaw(buf, sourceModel.updateManifest.lastAppliedUpdate);
  appendRaw(buf, sourceModel.updateManifest.nextMissingUpdate);

  appendRaw(buf, static_cast<std::uint32_t>(sourceModel.updateManifest.availableUpdates.size()));
  for(const auto &update : sourceModel.updateManifest.availableUpdates) {
    appendString(buf, update.name);
    appendRaw(buf, update.updateNumber);
    appendRaw(buf, update.sourceSize);
    appendRaw(buf, update.sourceTimestamp);
  }

  appendRaw(buf, static_cast<std::uint32_t>(sourceModel.updateManifest.appliedUpdates.size()));
  for(const auto updateNumber : sourceModel.updateManifest.appliedUpdates) {
    appendRaw(buf, updateNumber);
  }

  return buf;
}

std::vector<std::uint8_t> SencWriter::encodeS57FeatureSemanticsV2() const
{
  std::vector<std::uint8_t> buf;
  if(!shouldWriteV2Semantics()) {
    return buf;
  }

  const auto &features = m_s57SourceModel->features;
  appendRaw(buf, static_cast<std::uint32_t>(features.size()));

  std::uint64_t fallbackId = 0;
  for(const auto &feature : features) {
    const auto datasetFeatureId = makeS57DatasetFeatureId(feature, ++fallbackId);
    appendRaw(buf, datasetFeatureId);
    appendRaw(buf, feature.recordId);
    appendRaw(buf, feature.recordVersion);
    appendRaw(buf, feature.updateInstruction);
    appendRaw(buf, static_cast<std::uint8_t>(feature.primitive));
    appendRaw(buf, feature.classCode);
    appendString(buf, feature.classAcronym);

    const auto hasIdentity = static_cast<std::uint8_t>(feature.identity.has_value() && feature.identity->isValid());
    appendRaw(buf, hasIdentity);
    if(hasIdentity != 0) {
      appendRaw(buf, feature.identity->agency);
      appendRaw(buf, feature.identity->featureId);
      appendRaw(buf, feature.identity->featureSubdivision);
    }

    appendRaw(buf, static_cast<std::uint32_t>(feature.spatialPointers.size()));
    for(const auto &pointer : feature.spatialPointers) {
      appendRaw(buf, pointer.recordName);
      appendRaw(buf, pointer.recordId);
      appendRaw(buf, pointer.orientation);
      appendRaw(buf, pointer.usage);
      appendRaw(buf, pointer.mask);
    }

    appendAttributeMap(buf, feature.attributes);
    appendAttributeMap(buf, feature.nationalAttributes);
    appendGeometry(buf, feature.geometry);
  }

  return buf;
}

std::vector<std::uint8_t> SencWriter::encodeS57VectorRecordsV2() const
{
  std::vector<std::uint8_t> buf;
  if(!shouldWriteV2Semantics()) {
    return buf;
  }

  appendRaw(buf, static_cast<std::uint32_t>(m_s57SourceModel->vectors.size()));
  for(const auto &[_, vector] : m_s57SourceModel->vectors) {
    appendRaw(buf, vector.recordName);
    appendRaw(buf, vector.recordId);
    appendRaw(buf, vector.recordVersion);
    appendRaw(buf, vector.updateInstruction);
    appendRaw(buf, static_cast<std::uint32_t>(vector.coords.size()));
    for(const auto &coord : vector.coords) {
      appendRaw(buf, coord.lon);
      appendRaw(buf, coord.lat);
    }
  }

  return buf;
}

SourceManifest SencWriter::manifestForWrite(const chart_data::FeatureChartDataset &dataset) const
{
  if(m_manifest.has_value()) {
    return m_manifest.value();
  }

  if(m_s57SourceModel.has_value() && !m_s57SourceModel->sourceManifest.name.empty()) {
    return m_s57SourceModel->sourceManifest;
  }

  const auto &meta = dataset.meta();
  SourceManifest manifest;
  manifest.sourceType = meta.sourceType;
  manifest.name = meta.name;
  manifest.edition = meta.edition;
  manifest.update = meta.update;
  return manifest;
}

bool SencWriter::shouldWriteV2Semantics() const noexcept
{
  return m_formatVersion == kSencFormatVersionV2 && m_s57SourceModel.has_value();
}

}// namespace chart_view::runtime::senc
