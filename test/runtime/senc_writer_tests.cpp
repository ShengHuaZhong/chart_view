#include <catch2/catch_test_macros.hpp>

#include "senc/senc_types.hpp"
#include "senc/senc_writer.hpp"
#include "chart_data/feature_chart_dataset.hpp"
#include "chart_data/geometry.hpp"
#include "chart_data/dataset_meta.hpp"
#include "chart_data/feature.hpp"

#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

using namespace chart_view::runtime::senc;
using namespace chart_view::runtime::chart_data;

namespace {

// Helper: read a raw trivially-copyable value from a buffer at offset.
template<typename T>
T readRaw(const std::vector<std::uint8_t> &buf, std::size_t &offset)
{
  REQUIRE(offset + sizeof(T) <= buf.size());
  T val{};
  std::memcpy(&val, buf.data() + offset, sizeof(T));
  offset += sizeof(T);
  return val;
}

// Helper: read a length-prefixed string from buffer at offset.
std::string readString(const std::vector<std::uint8_t> &buf, std::size_t &offset)
{
  const auto len = readRaw<std::uint32_t>(buf, offset);
  REQUIRE(offset + len <= buf.size());
  std::string result(reinterpret_cast<const char *>(buf.data() + offset), len);
  offset += len;
  return result;
}

// CRC-32 matching the writer's implementation.
std::uint32_t crc32(const std::uint8_t *data, std::size_t size) noexcept
{
  std::uint32_t crc = 0xFFFFFFFFu;
  for (std::size_t i = 0; i < size; ++i) {
    crc = (crc ^ data[i]);
    for (int j = 0; j < 8; ++j) {
      if (crc & 1u)
        crc = (crc >> 1u) ^ 0xEDB88320u;
      else
        crc >>= 1u;
    }
  }
  return crc ^ 0xFFFFFFFFu;
}

FeatureChartDataset makeSyntheticDataset()
{
  FeatureChartDataset ds;

  DatasetMeta meta;
  meta.name = "US5TX01M";
  meta.sourceType = chart_view_chart_source_s57;
  meta.nativeScale = 50000.0;
  meta.extent = Extent{-97.5, 27.5, -96.0, 28.5};
  meta.usageBand = 4;
  meta.edition = 12;
  meta.update = 3;
  ds.setMeta(std::move(meta));

  // Point feature.
  Feature pt;
  pt.id = 1;
  pt.classCode = 159; // SOUNDG
  pt.classAcronym = "SOUNDG";
  pt.geometry = PointGeometry{{-96.5, 28.0}};
  pt.attributes["DRVAL1"] = 10.5;
  pt.attributes["QUASOU"] = std::int64_t{6};
  ds.addFeature(std::move(pt));

  // Line feature.
  Feature ln;
  ln.id = 2;
  ln.classCode = 71; // COALNE
  ln.classAcronym = "COALNE";
  ln.geometry = LineGeometry{{{-97.0, 27.5}, {-96.5, 28.0}, {-96.0, 28.5}}};
  ln.attributes["CATCOA"] = std::int64_t{1};
  ds.addFeature(std::move(ln));

  // Area feature.
  Feature ar;
  ar.id = 3;
  ar.classCode = 42; // DEPARE
  ar.classAcronym = "DEPARE";
  AreaGeometry areaGeo;
  areaGeo.exteriorRing = {{-97.0, 27.5}, {-96.0, 27.5}, {-96.0, 28.5}, {-97.0, 28.5}};
  areaGeo.interiorRings = {{{-96.8, 27.7}, {-96.2, 27.7}, {-96.2, 28.3}, {-96.8, 28.3}}};
  ar.geometry = std::move(areaGeo);
  ar.attributes["DRVAL1"] = 5.0;
  ar.attributes["DRVAL2"] = 20.0;
  ds.addFeature(std::move(ar));

  return ds;
}

}// namespace

TEST_CASE("SencWriter produces valid file header", "[senc][writer]")
{
  SencWriter writer;
  auto blob = writer.write(FeatureChartDataset{});

  REQUIRE(blob.size() >= sizeof(FileHeader));

  std::size_t offset = 0;
  auto fh = readRaw<FileHeader>(blob, offset);

  CHECK(fh.magic == kSencMagic);
  CHECK(fh.formatVersion == kSencFormatVersion);
  CHECK(fh.sectionCount == static_cast<std::uint32_t>(SectionType::kCount_));
  CHECK(fh.totalFileSize == blob.size());
}

TEST_CASE("SencWriter writes correct section count", "[senc][writer]")
{
  SencWriter writer;
  auto blob = writer.write(FeatureChartDataset{});

  std::size_t offset = 0;
  auto fh = readRaw<FileHeader>(blob, offset);

  const auto sectionCount = fh.sectionCount;
  CHECK(sectionCount == static_cast<std::uint32_t>(SectionType::kCount_));

  // Verify we can read all section descriptors.
  REQUIRE(blob.size() >= sizeof(FileHeader) + sectionCount * sizeof(SectionDesc));

  for (std::uint32_t i = 0; i < sectionCount; ++i) {
    auto desc = readRaw<SectionDesc>(blob, offset);
    CHECK(desc.type == static_cast<SectionType>(i));
    CHECK(desc.offset + desc.size <= fh.totalFileSize);
  }
}

TEST_CASE("SencWriter empty dataset produces valid structure", "[senc][writer]")
{
  SencWriter writer;
  FeatureChartDataset empty;
  auto blob = writer.write(empty);

  std::size_t offset = 0;
  auto fh = readRaw<FileHeader>(blob, offset);

  CHECK(fh.magic == kSencMagic);
  CHECK(fh.totalFileSize == blob.size());

  // All non-stub sections should exist, though some may be small.
  for (std::uint32_t i = 0; i < fh.sectionCount; ++i) {
    auto desc = readRaw<SectionDesc>(blob, offset);
    CHECK(desc.type == static_cast<SectionType>(i));
    // Payload range must be within file.
    CHECK(desc.offset + desc.size <= fh.totalFileSize);
  }
}

TEST_CASE("SencWriter section checksums are valid", "[senc][writer]")
{
  SencWriter writer;
  auto ds = makeSyntheticDataset();
  auto blob = writer.write(ds);

  std::size_t offset = 0;
  auto fh = readRaw<FileHeader>(blob, offset);

  for (std::uint32_t i = 0; i < fh.sectionCount; ++i) {
    auto desc = readRaw<SectionDesc>(blob, offset);
    if (desc.size > 0) {
      auto computed = crc32(blob.data() + desc.offset, desc.size);
      CHECK(computed == desc.checksum);
    } else {
      CHECK(desc.checksum == 0);
    }
  }
}

TEST_CASE("SencWriter section offsets are contiguous and non-overlapping", "[senc][writer]")
{
  SencWriter writer;
  auto ds = makeSyntheticDataset();
  auto blob = writer.write(ds);

  std::size_t offset = 0;
  auto fh = readRaw<FileHeader>(blob, offset);

  std::uint32_t expectedOffset = sizeof(FileHeader) + fh.sectionCount * sizeof(SectionDesc);

  for (std::uint32_t i = 0; i < fh.sectionCount; ++i) {
    auto desc = readRaw<SectionDesc>(blob, offset);
    CHECK(desc.offset == expectedOffset);
    expectedOffset += desc.size;
  }

  CHECK(expectedOffset == fh.totalFileSize);
}

TEST_CASE("SencWriter SourceManifest roundtrip", "[senc][writer]")
{
  SencWriter writer;
  auto ds = makeSyntheticDataset();
  auto blob = writer.write(ds);

  // Locate SourceManifest section.
  std::size_t offset = 0;
  auto fh = readRaw<FileHeader>(blob, offset);

  // Skip to SourceManifest descriptor (index 1).
  offset = sizeof(FileHeader) + sizeof(SectionDesc); // skip kFileHeader desc
  auto smDesc = readRaw<SectionDesc>(blob, offset);
  CHECK(smDesc.type == SectionType::kSourceManifest);
  REQUIRE(smDesc.size > 0);

  // Parse payload.
  std::size_t payloadOffset = smDesc.offset;
  auto srcType = readRaw<std::uint32_t>(blob, payloadOffset);
  auto name = readString(blob, payloadOffset);
  auto sourceSize = readRaw<std::uint64_t>(blob, payloadOffset);
  auto sourceTimestamp = readRaw<std::int64_t>(blob, payloadOffset);
  auto sourceHash = readRaw<std::uint32_t>(blob, payloadOffset);
  auto edition = readRaw<std::uint32_t>(blob, payloadOffset);
  auto update = readRaw<std::uint32_t>(blob, payloadOffset);

  CHECK(srcType == static_cast<std::uint32_t>(chart_view_chart_source_s57));
  CHECK(name == "US5TX01M");
  // Derived from DatasetMeta -- sourceSize/timestamp/hash are 0 (unknown).
  CHECK(sourceSize == 0);
  CHECK(sourceTimestamp == 0);
  CHECK(sourceHash == 0);
  CHECK(edition == 12);
  CHECK(update == 3);
}

TEST_CASE("SencWriter DatasetMeta roundtrip", "[senc][writer]")
{
  SencWriter writer;
  auto ds = makeSyntheticDataset();
  auto blob = writer.write(ds);

  // Locate DatasetMeta descriptor (index 2).
  std::size_t offset = sizeof(FileHeader) + 2 * sizeof(SectionDesc);
  auto dmDesc = readRaw<SectionDesc>(blob, offset);
  CHECK(dmDesc.type == SectionType::kDatasetMeta);
  REQUIRE(dmDesc.size > 0);

  std::size_t po = dmDesc.offset;
  auto name = readString(blob, po);
  auto srcType = readRaw<std::uint32_t>(blob, po);
  auto nativeScale = readRaw<double>(blob, po);
  auto minLon = readRaw<double>(blob, po);
  auto minLat = readRaw<double>(blob, po);
  auto maxLon = readRaw<double>(blob, po);
  auto maxLat = readRaw<double>(blob, po);
  auto usageBand = readRaw<std::uint32_t>(blob, po);
  auto edition = readRaw<std::uint32_t>(blob, po);
  auto update = readRaw<std::uint32_t>(blob, po);

  CHECK(name == "US5TX01M");
  CHECK(srcType == static_cast<std::uint32_t>(chart_view_chart_source_s57));
  CHECK(nativeScale == 50000.0);
  CHECK(minLon == -97.5);
  CHECK(minLat == 27.5);
  CHECK(maxLon == -96.0);
  CHECK(maxLat == 28.5);
  CHECK(usageBand == 4);
  CHECK(edition == 12);
  CHECK(update == 3);
}

TEST_CASE("SencWriter FeatureTable roundtrip", "[senc][writer]")
{
  SencWriter writer;
  auto ds = makeSyntheticDataset();
  auto blob = writer.write(ds);

  // Locate FeatureTable descriptor (index 3).
  std::size_t offset = sizeof(FileHeader) + 3 * sizeof(SectionDesc);
  auto ftDesc = readRaw<SectionDesc>(blob, offset);
  CHECK(ftDesc.type == SectionType::kFeatureTable);
  REQUIRE(ftDesc.size > 0);

  std::size_t po = ftDesc.offset;
  auto featureCount = readRaw<std::uint32_t>(blob, po);
  CHECK(featureCount == 3);

  // Feature 0: SOUNDG (point)
  auto id0 = readRaw<std::uint64_t>(blob, po);
  auto cc0 = readRaw<std::uint32_t>(blob, po);
  auto acro0 = readString(blob, po);
  auto gtype0 = readRaw<std::uint8_t>(blob, po);
  auto ac0 = readRaw<std::uint32_t>(blob, po);

  CHECK(id0 == 1);
  CHECK(cc0 == 159);
  CHECK(acro0 == "SOUNDG");
  CHECK(gtype0 == static_cast<std::uint8_t>(GeometryType::kPoint));
  CHECK(ac0 == 2); // DRVAL1, QUASOU

  // Skip attribute names for feature 0.
  for (std::uint32_t a = 0; a < ac0; ++a) {
    readString(blob, po); // consume
  }

  // Feature 1: COALNE (line)
  auto id1 = readRaw<std::uint64_t>(blob, po);
  CHECK(id1 == 2);
  auto cc1 = readRaw<std::uint32_t>(blob, po);
  CHECK(cc1 == 71);
}

TEST_CASE("SencWriter GeometryBlob roundtrip", "[senc][writer]")
{
  SencWriter writer;
  auto ds = makeSyntheticDataset();
  auto blob = writer.write(ds);

  // Locate GeometryBlob descriptor (index 4).
  std::size_t offset = sizeof(FileHeader) + 4 * sizeof(SectionDesc);
  auto gbDesc = readRaw<SectionDesc>(blob, offset);
  CHECK(gbDesc.type == SectionType::kGeometryBlob);
  REQUIRE(gbDesc.size > 0);

  std::size_t po = gbDesc.offset;

  // Feature 0: Point
  auto gtype0 = readRaw<std::uint8_t>(blob, po);
  CHECK(gtype0 == static_cast<std::uint8_t>(GeometryType::kPoint));
  auto lon0 = readRaw<double>(blob, po);
  auto lat0 = readRaw<double>(blob, po);
  CHECK(lon0 == -96.5);
  CHECK(lat0 == 28.0);

  // Feature 1: Line
  auto gtype1 = readRaw<std::uint8_t>(blob, po);
  CHECK(gtype1 == static_cast<std::uint8_t>(GeometryType::kLine));
  auto vertCount = readRaw<std::uint32_t>(blob, po);
  CHECK(vertCount == 3);
  // First vertex.
  auto lv0lon = readRaw<double>(blob, po);
  auto lv0lat = readRaw<double>(blob, po);
  CHECK(lv0lon == -97.0);
  CHECK(lv0lat == 27.5);
  // Skip remaining vertices.
  po += 2 * 2 * sizeof(double);

  // Feature 2: Area
  auto gtype2 = readRaw<std::uint8_t>(blob, po);
  CHECK(gtype2 == static_cast<std::uint8_t>(GeometryType::kArea));
  auto extCount = readRaw<std::uint32_t>(blob, po);
  CHECK(extCount == 4);
}

TEST_CASE("SencWriter blob size matches totalFileSize", "[senc][writer]")
{
  SencWriter writer;
  auto ds = makeSyntheticDataset();
  auto blob = writer.write(ds);

  std::size_t offset = 0;
  auto fh = readRaw<FileHeader>(blob, offset);
  CHECK(blob.size() == fh.totalFileSize);
}

TEST_CASE("SencWriter deterministic output", "[senc][writer]")
{
  SencWriter writer;
  auto ds1 = makeSyntheticDataset();
  auto ds2 = makeSyntheticDataset();
  auto blob1 = writer.write(ds1);
  auto blob2 = writer.write(ds2);
  CHECK(blob1 == blob2);
}
