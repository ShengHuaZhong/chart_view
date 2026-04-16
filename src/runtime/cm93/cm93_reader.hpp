#ifndef CHART_VIEW_RUNTIME_CM93_CM93_READER_HPP
#define CHART_VIEW_RUNTIME_CM93_CM93_READER_HPP

#include "../chart_data/feature_chart_dataset.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace chart_view::runtime::cm93 {

enum class Cm93ExtentSource : std::uint8_t
{
  kNone = 0,
  kGeometry = 1,
  kHeader = 2,
  kCellNameFallback = 3
};

// Result of reading a CM93 chart cell.
struct Cm93ReadResult
{
  bool ok{false};
  std::string error;
  chart_data::FeatureChartDataset dataset;
  Cm93ExtentSource extentSource{Cm93ExtentSource::kNone};
};

// Reads a single CM93 cell file and produces a FeatureChartDataset.
// A CM93 "chart" can be:
//   - A single cell file (e.g. "00540000.C")
//   - A cell directory containing multiple cell files
class Cm93Reader
{
public:
  Cm93Reader() = default;

  // Read a single CM93 cell file.
  [[nodiscard]] Cm93ReadResult read(const std::string &path) const;

  // Read from an in-memory buffer (single cell).
  [[nodiscard]] Cm93ReadResult readFromMemory(
    std::span<const std::uint8_t> data,
    const std::string &cellName) const;

  // Read all cells under a CM93 root directory (top-level with A/C/Z/... subdirs).
  // Reads the first available cell for quick smoke testing.
  [[nodiscard]] Cm93ReadResult readFirstCell(const std::string &cm93Root) const;
};

}// namespace chart_view::runtime::cm93

#endif
