#include "s101_reader.hpp"

#include "../chart_data/feature.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string_view>

namespace chart_view::runtime::s101 {

namespace {

std::string trimCopy(std::string_view value)
{
  const auto first = value.find_first_not_of(" \t\r\n");
  if(first == std::string_view::npos) {
    return {};
  }

  const auto last = value.find_last_not_of(" \t\r\n");
  return std::string(value.substr(first, last - first + 1));
}

bool parseDouble(std::string_view value, double &outValue)
{
  const auto trimmed = trimCopy(value);
  if(trimmed.empty()) {
    return false;
  }

  char *end = nullptr;
  errno = 0;
  const double parsed = std::strtod(trimmed.c_str(), &end);
  if(errno != 0 || end == trimmed.c_str() || *end != '\0') {
    return false;
  }

  outValue = parsed;
  return true;
}

bool parseCoordinate(std::string_view value, chart_data::Coordinate &outCoord)
{
  const auto commaPos = value.find(',');
  if(commaPos == std::string_view::npos) {
    return false;
  }

  double lon = 0.0;
  double lat = 0.0;
  if(!parseDouble(value.substr(0, commaPos), lon) ||
     !parseDouble(value.substr(commaPos + 1), lat)) {
    return false;
  }

  outCoord = {lon, lat};
  return true;
}

template<typename EmitFn>
bool parseCoordinateList(std::string_view value, EmitFn emit)
{
  std::size_t start = 0;
  bool sawCoordinate = false;

  while(start <= value.size()) {
    const auto end = value.find(';', start);
    const auto token = value.substr(start, end == std::string_view::npos ? value.size() - start : end - start);

    const auto trimmed = trimCopy(token);
    if(!trimmed.empty()) {
      chart_data::Coordinate coord;
      if(!parseCoordinate(trimmed, coord)) {
        return false;
      }
      emit(coord);
      sawCoordinate = true;
    }

    if(end == std::string_view::npos) {
      break;
    }
    start = end + 1;
  }

  return sawCoordinate;
}

void expandExtent(chart_data::Extent &extent, const chart_data::Coordinate &coord)
{
  extent.minLon = std::min(extent.minLon, coord.lon);
  extent.minLat = std::min(extent.minLat, coord.lat);
  extent.maxLon = std::max(extent.maxLon, coord.lon);
  extent.maxLat = std::max(extent.maxLat, coord.lat);
}

bool parseSyntheticSmokeDataset(std::string_view text,
                                const std::string &fallbackName,
                                chart_data::FeatureChartDataset &outDataset,
                                std::string &outError)
{
  constexpr std::string_view kHeader = "S101SMOKE";
  if(!text.starts_with(kHeader)) {
    return false;
  }

  chart_data::DatasetMeta meta;
  meta.name = fallbackName.empty() ? "s101_smoke" : fallbackName;
  meta.sourceType = chart_view_chart_source_s101;
  meta.nativeScale = 12000.0;

  chart_data::Extent extent{
    std::numeric_limits<double>::max(),
    std::numeric_limits<double>::max(),
    std::numeric_limits<double>::lowest(),
    std::numeric_limits<double>::lowest()};

  std::uint64_t nextId = 1;
  std::size_t lineStart = text.find('\n');
  if(lineStart == std::string_view::npos) {
    outError = "synthetic S-101 smoke data is missing body lines";
    return true;
  }
  ++lineStart;

  while(lineStart <= text.size()) {
    const auto lineEnd = text.find('\n', lineStart);
    const auto rawLine = text.substr(
      lineStart,
      lineEnd == std::string_view::npos ? text.size() - lineStart : lineEnd - lineStart);
    const auto line = trimCopy(rawLine);

    if(!line.empty() && !line.starts_with('#')) {
      const auto equalsPos = line.find('=');
      if(equalsPos == std::string::npos) {
        outError = "synthetic S-101 smoke line is missing '='";
        return true;
      }

      const auto key = trimCopy(std::string_view(line).substr(0, equalsPos));
      const auto value = trimCopy(std::string_view(line).substr(equalsPos + 1));

      if(key == "name") {
        if(value.empty()) {
          outError = "synthetic S-101 smoke name is empty";
          return true;
        }
        meta.name = value;
      } else if(key == "native_scale") {
        if(!parseDouble(value, meta.nativeScale) || meta.nativeScale <= 0.0) {
          outError = "synthetic S-101 smoke native_scale is invalid";
          return true;
        }
      } else if(key == "point") {
        chart_data::Coordinate coord;
        if(!parseCoordinate(value, coord)) {
          outError = "synthetic S-101 smoke point is invalid";
          return true;
        }

        chart_data::Feature feature;
        feature.id = nextId++;
        feature.classCode = 129;
        feature.classAcronym = "Sounding";
        feature.geometry = chart_data::PointGeometry{coord};
        outDataset.addFeature(std::move(feature));
        expandExtent(extent, coord);
      } else if(key == "line") {
        chart_data::LineGeometry lineGeometry;
        if(!parseCoordinateList(value, [&](const chart_data::Coordinate &coord) {
             lineGeometry.vertices.push_back(coord);
             expandExtent(extent, coord);
           }) ||
           lineGeometry.vertices.size() < 2) {
          outError = "synthetic S-101 smoke line requires at least 2 coordinates";
          return true;
        }

        chart_data::Feature feature;
        feature.id = nextId++;
        feature.classCode = 42;
        feature.classAcronym = "DepthContour";
        feature.geometry = std::move(lineGeometry);
        outDataset.addFeature(std::move(feature));
      } else if(key == "area") {
        chart_data::AreaGeometry areaGeometry;
        if(!parseCoordinateList(value, [&](const chart_data::Coordinate &coord) {
             areaGeometry.exteriorRing.push_back(coord);
             expandExtent(extent, coord);
           }) ||
           areaGeometry.exteriorRing.size() < 3) {
          outError = "synthetic S-101 smoke area requires at least 3 coordinates";
          return true;
        }

        chart_data::Feature feature;
        feature.id = nextId++;
        feature.classCode = 125;
        feature.classAcronym = "DepthArea";
        feature.geometry = std::move(areaGeometry);
        outDataset.addFeature(std::move(feature));
      } else {
        outError = "synthetic S-101 smoke key is not supported: " + key;
        return true;
      }
    }

    if(lineEnd == std::string_view::npos) {
      break;
    }
    lineStart = lineEnd + 1;
  }

  if(outDataset.empty()) {
    outError = "synthetic S-101 smoke dataset has no features";
    return true;
  }

  meta.extent = extent;
  outDataset.setMeta(std::move(meta));
  outError.clear();
  return true;
}

}// namespace

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

  const auto textView = std::string_view(
    reinterpret_cast<const char *>(data.data()),
    data.size());
  if(parseSyntheticSmokeDataset(textView, name, result.dataset, result.error)) {
    result.ok = result.error.empty();
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
