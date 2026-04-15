#ifndef CHART_VIEW_RUNTIME_SENC_SENC_TYPES_HPP
#define CHART_VIEW_RUNTIME_SENC_SENC_TYPES_HPP

#include <cstdint>

namespace chart_view::runtime::senc {

// Current SENC binary format version.
inline constexpr std::uint32_t kSencFormatVersion = 1;

// Magic bytes at the start of every SENC file: "SENC" in ASCII.
inline constexpr std::uint32_t kSencMagic = 0x434E4553u;  // 'S','E','N','C' little-endian

// Section type identifiers.
enum class SectionType : std::uint16_t
{
  kFileHeader = 0,
  kSourceManifest = 1,
  kDatasetMeta = 2,
  kFeatureTable = 3,
  kGeometryBlob = 4,
  kAttributeBlob = 5,
  kSpatialIndex = 6,
  kRenderCache = 7,
  kPickIndex = 8,
  kStringTable = 9,

  kCount_ = 10  // sentinel -- total mandatory sections
};

// Section descriptor in the section table.
// Appears once per section in the file header area.
struct SectionDesc
{
  SectionType type;
  std::uint16_t reserved{0};
  std::uint32_t offset{0};   // byte offset from start of file
  std::uint32_t size{0};     // section payload size in bytes
  std::uint32_t checksum{0}; // CRC-32 of section payload
};

static_assert(sizeof(SectionDesc) == 16, "SectionDesc must be 16 bytes for binary layout stability");

// File header -- fixed layout at the start of every SENC file.
struct FileHeader
{
  std::uint32_t magic{kSencMagic};
  std::uint32_t formatVersion{kSencFormatVersion};
  std::uint32_t sectionCount{0};
  std::uint32_t totalFileSize{0};
  // The section table follows immediately after the FileHeader.
};

static_assert(sizeof(FileHeader) == 16, "FileHeader must be 16 bytes for binary layout stability");

}// namespace chart_view::runtime::senc

#endif
