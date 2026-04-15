#ifndef CHART_VIEW_RUNTIME_S101_S101_NORMALIZER_HPP
#define CHART_VIEW_RUNTIME_S101_S101_NORMALIZER_HPP

#include "s101_reader.hpp"

#include <string>

namespace chart_view::runtime::s101 {

// Thin wrapper around S101Reader for the normalizer pipeline interface.
// Phase-1 scaffold; full implementation deferred to when S-101 test data is available.
class S101Normalizer
{
public:
  S101Normalizer() = default;

  [[nodiscard]] S101ReadResult normalize(const std::string &path) const
  {
    return m_reader.read(path);
  }

  [[nodiscard]] S101ReadResult normalizeFromMemory(
    std::span<const std::uint8_t> data,
    const std::string &name = {}) const
  {
    return m_reader.readFromMemory(data, name);
  }

private:
  S101Reader m_reader;
};

}// namespace chart_view::runtime::s101

#endif
