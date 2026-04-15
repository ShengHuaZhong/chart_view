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
  fh.formatVersion = kSencFormatVersion;
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
  // kSpatialIndex  (index 6): stub -- empty for Phase 1
  // kRenderCache   (index 7): stub -- empty for Phase 1
  // kPickIndex     (index 8): stub -- empty for Phase 1
  // kStringTable   (index 9): stub -- empty for Phase 1

  return payloads;
}

std::vector<std::uint8_t> SencWriter::encodeSourceManifest(
  const chart_data::FeatureChartDataset &dataset) const
{
  std::vector<std::uint8_t> buf;

  if (m_manifest.has_value()) {
    const auto &m = m_manifest.value();
    appendRaw(buf, static_cast<std::uint32_t>(m.sourceType));
    appendString(buf, m.name);
    appendRaw(buf, m.sourceSize);
    appendRaw(buf, m.sourceTimestamp);
    appendRaw(buf, m.sourceHash);
    appendRaw(buf, m.edition);
    appendRaw(buf, m.update);
  } else {
    // Derive minimal manifest from dataset metadata.
    const auto &meta = dataset.meta();
    appendRaw(buf, static_cast<std::uint32_t>(meta.sourceType));
    appendString(buf, meta.name);
    appendRaw(buf, std::uint64_t{0}); // sourceSize unknown
    appendRaw(buf, std::int64_t{0});  // sourceTimestamp unknown
    appendRaw(buf, std::uint32_t{0}); // sourceHash unknown
    appendRaw(buf, meta.edition);
    appendRaw(buf, meta.update);
  }

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

      // Type tag: 0 = int64, 1 = double, 2 = string
      const auto typeTag = static_cast<std::uint8_t>(val.index());
      appendRaw(buf, typeTag);

      if (typeTag == 0) {
        appendRaw(buf, std::get<std::int64_t>(val));
      } else if (typeTag == 1) {
        appendRaw(buf, std::get<double>(val));
      } else {
        appendString(buf, std::get<std::string>(val));
      }
    }
  }

  return buf;
}

}// namespace chart_view::runtime::senc
