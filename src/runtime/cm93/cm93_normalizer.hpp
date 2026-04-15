#ifndef CHART_VIEW_RUNTIME_CM93_CM93_NORMALIZER_HPP
#define CHART_VIEW_RUNTIME_CM93_CM93_NORMALIZER_HPP

#include "cm93_reader.hpp"

#include <string>

namespace chart_view::runtime::cm93 {

// Thin wrapper around Cm93Reader for the normalizer pipeline interface.
class Cm93Normalizer
{
public:
  Cm93Normalizer() = default;

  // Normalize a single CM93 cell file.
  [[nodiscard]] Cm93ReadResult normalize(const std::string &path) const
  {
    return m_reader.read(path);
  }

  // Normalize from an in-memory buffer.
  [[nodiscard]] Cm93ReadResult normalizeFromMemory(
    std::span<const std::uint8_t> data,
    const std::string &cellName = {}) const
  {
    return m_reader.readFromMemory(data, cellName);
  }

  // Read the first available cell from a CM93 root directory.
  [[nodiscard]] Cm93ReadResult normalizeFirstCell(const std::string &cm93Root) const
  {
    return m_reader.readFirstCell(cm93Root);
  }

private:
  Cm93Reader m_reader;
};

}// namespace chart_view::runtime::cm93

#endif
