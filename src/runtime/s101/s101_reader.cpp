#include "s101_reader.hpp"

#include <filesystem>
#include <fstream>

namespace chart_view::runtime::s101 {

S101ReadResult S101Reader::read(const std::string &path) const
{
  std::ifstream ifs(path, std::ios::binary | std::ios::ate);
  if (!ifs) {
    return {false, "cannot open file: " + path, {}};
  }

  const auto size = static_cast<std::size_t>(ifs.tellg());
  ifs.seekg(0, std::ios::beg);

  std::vector<std::uint8_t> data(size);
  if (!ifs.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(size))) {
    return {false, "failed to read file: " + path, {}};
  }

  auto fileName = std::filesystem::path(path).stem().string();
  return readFromMemory(data, fileName);
}

S101ReadResult S101Reader::readFromMemory(
  std::span<const std::uint8_t> data,
  const std::string &name) const
{
  S101ReadResult result;

  if (data.size() < 24) {
    result.error = "S-101 data too small";
    return result;
  }

  // Phase-1 scaffold: produce an empty but valid dataset.
  // Full S-101 GML parsing will be implemented when test data is available.
  chart_data::DatasetMeta meta;
  meta.name = name.empty() ? "s101" : name;
  meta.sourceType = chart_view_chart_source_s101;

  result.dataset.setMeta(std::move(meta));
  result.ok = true;
  result.error = "S-101 parsing is scaffold-only (no features extracted)";
  return result;
}

}// namespace chart_view::runtime::s101
