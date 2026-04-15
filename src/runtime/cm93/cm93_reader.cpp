#include "cm93_reader.hpp"
#include "cm93_decode.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace chart_view::runtime::cm93 {

namespace {

// Extract the detail level character from a cell filename.
// e.g. "00540000.C" -> 'C', "00300000.A" -> 'A'
char detailLevelFromName(const std::string &cellName)
{
  auto dotPos = cellName.rfind('.');
  if (dotPos != std::string::npos && dotPos + 1 < cellName.size()) {
    return cellName[dotPos + 1];
  }
  return 'C'; // default
}

// Extract base name (without path).
std::string baseNameOf(const std::string &path)
{
  return std::filesystem::path(path).filename().string();
}

// Convert decoded CM93 features into a FeatureChartDataset.
chart_data::FeatureChartDataset convertToDataset(
  const Cm93Cell &cell,
  const std::string &cellName)
{
  chart_data::FeatureChartDataset ds;

  chart_data::DatasetMeta meta;
  meta.name = cellName;
  meta.sourceType = chart_view_chart_source_cm93;

  char level = detailLevelFromName(cellName);
  meta.nativeScale = cm93ScaleFactor(level);

  chart_data::Extent extent;
  bool extentInitialized = false;
  std::uint64_t featureId = 0;

  double cellOriginLon = 0.0;
  double cellOriginLat = 0.0;

  // Try to extract cell origin from filename.
  std::string baseName = cellName;
  auto dotPos = baseName.rfind('.');
  if (dotPos != std::string::npos) {
    baseName = baseName.substr(0, dotPos);
  }
  cm93CellOrigin(baseName, level, cellOriginLon, cellOriginLat);

  double sf = 1.0; // coordinate conversion scale is handled in cm93ToLonLat

  for (const auto &feat : cell.features) {
    ++featureId;
    chart_data::Feature f;
    f.id = featureId;

    // Map object class code.
    if (feat.objectClassIndex < cell.objectClassCodes.size()) {
      f.classCode = cell.objectClassCodes[feat.objectClassIndex];
    }
    f.classAcronym = "CM93_" + std::to_string(f.classCode);

    // Convert geometry.
    auto updateExtent = [&](double lon, double lat) {
      if (!extentInitialized) {
        extent.minLon = extent.maxLon = lon;
        extent.minLat = extent.maxLat = lat;
        extentInitialized = true;
      } else {
        extent.minLon = std::min(extent.minLon, lon);
        extent.maxLon = std::max(extent.maxLon, lon);
        extent.minLat = std::min(extent.minLat, lat);
        extent.maxLat = std::max(extent.maxLat, lat);
      }
    };

    if (feat.geometryPrimitive == 1) {
      // Point.
      chart_data::PointGeometry pt;
      if (!feat.geometry.empty() && !feat.geometry[0].empty()) {
        double lon = 0.0;
        double lat = 0.0;
        cm93ToLonLat(feat.geometry[0][0].x, feat.geometry[0][0].y,
                     cellOriginLon, cellOriginLat, sf, lon, lat);
        pt.position = {lon, lat};
        updateExtent(lon, lat);
      }
      f.geometry = pt;
    } else if (feat.geometryPrimitive == 2) {
      // Line.
      chart_data::LineGeometry line;
      for (const auto &part : feat.geometry) {
        for (const auto &p : part) {
          double lon = 0.0;
          double lat = 0.0;
          cm93ToLonLat(p.x, p.y, cellOriginLon, cellOriginLat, sf, lon, lat);
          line.vertices.push_back({lon, lat});
          updateExtent(lon, lat);
        }
      }
      f.geometry = line;
    } else {
      // Area (primitive 4 or other).
      chart_data::AreaGeometry area;
      bool isFirst = true;
      for (const auto &part : feat.geometry) {
        std::vector<chart_data::Coordinate> ring;
        for (const auto &p : part) {
          double lon = 0.0;
          double lat = 0.0;
          cm93ToLonLat(p.x, p.y, cellOriginLon, cellOriginLat, sf, lon, lat);
          ring.push_back({lon, lat});
          updateExtent(lon, lat);
        }
        if (isFirst) {
          area.exteriorRing = std::move(ring);
          isFirst = false;
        } else {
          area.interiorRings.push_back(std::move(ring));
        }
      }
      f.geometry = area;
    }

    // Convert attributes.
    for (const auto &[code, val] : feat.attributes) {
      std::string key = "A" + std::to_string(code);
      // Try numeric parse.
      if (!val.empty()) {
        char *end = nullptr;
        double dval = std::strtod(val.c_str(), &end);
        if (end == val.c_str() + val.size()) {
          auto intVal = static_cast<std::int64_t>(dval);
          if (static_cast<double>(intVal) == dval) {
            f.attributes[key] = intVal;
          } else {
            f.attributes[key] = dval;
          }
        } else {
          f.attributes[key] = val;
        }
      }
    }

    ds.addFeature(std::move(f));
  }

  meta.extent = extent;
  ds.setMeta(std::move(meta));
  return ds;
}

}// namespace

Cm93ReadResult Cm93Reader::read(const std::string &path) const
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

  auto cellName = baseNameOf(path);
  return readFromMemory(data, cellName);
}

Cm93ReadResult Cm93Reader::readFromMemory(
  std::span<const std::uint8_t> data,
  const std::string &cellName) const
{
  Cm93ReadResult result;

  // Make a mutable copy for decryption.
  std::vector<std::uint8_t> buf(data.begin(), data.end());

  auto decoded = decodeCm93Cell(std::move(buf), cellName);
  if (!decoded.ok) {
    result.error = decoded.error;
    return result;
  }

  result.dataset = convertToDataset(decoded.cell, cellName);
  result.ok = true;
  return result;
}

Cm93ReadResult Cm93Reader::readFirstCell(const std::string &cm93Root) const
{
  namespace fs = std::filesystem;

  if (!fs::exists(cm93Root)) {
    return {false, "CM93 root does not exist: " + cm93Root, {}};
  }

  // Search through detail level directories (A, B, C, ..., Z) in each cell folder.
  // Look for the first available cell file.
  for (const auto &cellDir : fs::directory_iterator(cm93Root)) {
    if (!cellDir.is_directory()) continue;

    // Skip non-cell directories (e.g. those without 8-digit names).
    auto dirName = cellDir.path().filename().string();
    if (dirName.size() != 8) continue;

    // Look for cell files inside detail-level subdirectories.
    for (const auto &levelDir : fs::directory_iterator(cellDir.path())) {
      if (!levelDir.is_directory()) continue;
      auto levelName = levelDir.path().filename().string();
      if (levelName.size() != 1) continue;

      for (const auto &cellFile : fs::directory_iterator(levelDir.path())) {
        if (!cellFile.is_regular_file()) continue;
        auto ext = cellFile.path().extension().string();
        if (ext.size() == 2 && ext[0] == '.') {
          // This looks like a CM93 cell file (e.g. .C, .A, .Z).
          return read(cellFile.path().string());
        }
      }
    }
  }

  return {false, "no CM93 cell files found under: " + cm93Root, {}};
}

}// namespace chart_view::runtime::cm93
