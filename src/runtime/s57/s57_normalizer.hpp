#ifndef CHART_VIEW_RUNTIME_S57_S57_NORMALIZER_HPP
#define CHART_VIEW_RUNTIME_S57_S57_NORMALIZER_HPP

#include "s57_reader.hpp"

#include <string>

namespace chart_view::runtime::s57 {

// Reads a single S-57 .000 chart and normalizes it into a FeatureChartDataset.
// Thin wrapper around S57Reader for the normalizer interface expected by the pipeline.
class S57Normalizer
{
public:
  S57Normalizer() = default;

  // Normalize a single .000 file on disk.
  [[nodiscard]] S57ReadResult normalize(const std::string &path) const
  {
    return m_reader.read(path);
  }

  // Normalize from an in-memory buffer.
  [[nodiscard]] S57ReadResult normalizeFromMemory(
    std::span<const std::uint8_t> data,
    const std::string &name = {}) const
  {
    return m_reader.readFromMemory(data, name);
  }

private:
  S57Reader m_reader;
};

}// namespace chart_view::runtime::s57

#endif
