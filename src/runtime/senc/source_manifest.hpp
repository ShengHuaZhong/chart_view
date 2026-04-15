#ifndef CHART_VIEW_RUNTIME_SENC_SOURCE_MANIFEST_HPP
#define CHART_VIEW_RUNTIME_SENC_SOURCE_MANIFEST_HPP

#include <chart_view/runtime/chart_runtime_types.h>

#include <cstdint>
#include <string>

namespace chart_view::runtime::senc {

// Describes the source file from which a SENC was built.
// Used to detect when a SENC is stale and needs rebuild.
struct SourceManifest
{
  // Source file name (e.g. "US5TX01M.000").
  std::string name;

  // Source format.
  chart_view_chart_source_type_t sourceType{chart_view_chart_source_unknown};

  // File size of the original source in bytes (0 = unknown).
  std::uint64_t sourceSize{0};

  // Modification timestamp of the source file (seconds since epoch, 0 = unknown).
  std::int64_t sourceTimestamp{0};

  // Hash of the source file content (CRC-32 or similar, 0 = not computed).
  std::uint32_t sourceHash{0};

  // Edition/update number from source metadata.
  std::uint32_t edition{0};
  std::uint32_t update{0};
};

}// namespace chart_view::runtime::senc

#endif
