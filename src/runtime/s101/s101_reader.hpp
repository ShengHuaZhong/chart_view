#ifndef CHART_VIEW_RUNTIME_S101_S101_READER_HPP
#define CHART_VIEW_RUNTIME_S101_S101_READER_HPP

#include "../chart_data/feature_chart_dataset.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace chart_view::runtime::s101 {

// Result of reading an S-101 chart file.
struct S101ReadResult
{
  bool ok{false};
  std::string error;
  chart_data::FeatureChartDataset dataset;
};

// Reads a single S-101 dataset and produces a FeatureChartDataset.
// S-101 uses ISO 8211 encoding with GML geometry.
// This is a Phase-1 scaffold; full GML parsing is deferred until
// real S-101 test data is available.
class S101Reader
{
public:
  S101Reader() = default;

  [[nodiscard]] S101ReadResult read(const std::string &path) const;
  [[nodiscard]] S101ReadResult readFromMemory(
    std::span<const std::uint8_t> data,
    const std::string &name = {}) const;
};

}// namespace chart_view::runtime::s101

#endif
