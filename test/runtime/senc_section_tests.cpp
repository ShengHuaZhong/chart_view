#include <catch2/catch_test_macros.hpp>

#include "senc/senc_types.hpp"

using namespace chart_view::runtime::senc;

TEST_CASE("FileHeader has correct magic and format version", "[senc][layout]")
{
  FileHeader hdr;
  REQUIRE(hdr.magic == kSencMagic);
  REQUIRE(hdr.formatVersion == kSencFormatVersion);
  REQUIRE(hdr.sectionCount == 0);
  REQUIRE(hdr.totalFileSize == 0);
}

TEST_CASE("FileHeader is 16 bytes", "[senc][layout]")
{
  STATIC_REQUIRE(sizeof(FileHeader) == 16);
}

TEST_CASE("SectionDesc is 16 bytes", "[senc][layout]")
{
  STATIC_REQUIRE(sizeof(SectionDesc) == 16);
}

TEST_CASE("SectionType enum covers all 10 mandatory sections", "[senc][layout]")
{
  STATIC_REQUIRE(static_cast<int>(SectionType::kCount_) == 10);

  // Verify ordering.
  REQUIRE(static_cast<int>(SectionType::kFileHeader) == 0);
  REQUIRE(static_cast<int>(SectionType::kSourceManifest) == 1);
  REQUIRE(static_cast<int>(SectionType::kDatasetMeta) == 2);
  REQUIRE(static_cast<int>(SectionType::kFeatureTable) == 3);
  REQUIRE(static_cast<int>(SectionType::kGeometryBlob) == 4);
  REQUIRE(static_cast<int>(SectionType::kAttributeBlob) == 5);
  REQUIRE(static_cast<int>(SectionType::kSpatialIndex) == 6);
  REQUIRE(static_cast<int>(SectionType::kRenderCache) == 7);
  REQUIRE(static_cast<int>(SectionType::kPickIndex) == 8);
  REQUIRE(static_cast<int>(SectionType::kStringTable) == 9);
}

TEST_CASE("SectionDesc default values", "[senc][layout]")
{
  SectionDesc desc{};
  REQUIRE(desc.offset == 0);
  REQUIRE(desc.size == 0);
  REQUIRE(desc.checksum == 0);
  REQUIRE(desc.reserved == 0);
}

TEST_CASE("kSencMagic encodes SENC in little-endian", "[senc][layout]")
{
  auto magic = kSencMagic;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto *bytes = reinterpret_cast<const unsigned char *>(&magic);
  REQUIRE(bytes[0] == 'S');
  REQUIRE(bytes[1] == 'E');
  REQUIRE(bytes[2] == 'N');
  REQUIRE(bytes[3] == 'C');
}

TEST_CASE("Format version is 1", "[senc][layout]")
{
  STATIC_REQUIRE(kSencFormatVersion == kSencFormatVersionV1);
  STATIC_REQUIRE(kSencFormatVersionV1 == 1);
  STATIC_REQUIRE(kSencFormatVersionV2 == 2);
}
