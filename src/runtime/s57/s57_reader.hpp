#ifndef CHART_VIEW_RUNTIME_S57_S57_READER_HPP
#define CHART_VIEW_RUNTIME_S57_S57_READER_HPP

#include "../chart_data/feature_chart_dataset.hpp"
#include "s57_source_model.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace chart_view::runtime::s57 {

// Result of reading an S-57 chart file.
struct S57ReadResult
{
  bool ok{false};
  std::string error;
  chart_data::FeatureChartDataset dataset;
  S57SourceModel sourceModel;
};

// Reads a single S-57 .000 file and produces a FeatureChartDataset.
class S57Reader
{
public:
  S57Reader() = default;

  [[nodiscard]] S57ReadResult read(const std::string &path) const;
  [[nodiscard]] S57ReadResult readFromMemory(
    std::span<const std::uint8_t> data,
    const std::string &name = {}) const;
};

}// namespace chart_view::runtime::s57

#endif
