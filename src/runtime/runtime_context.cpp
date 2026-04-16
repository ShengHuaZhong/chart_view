#include "runtime_context.hpp"

#include "cm93/cm93_reader.hpp"
#include "label_layout.hpp"
#include "quilt/quilt_planner.hpp"
#include "quilt/zoom_policy.hpp"
#include "s101/s101_reader.hpp"
#include "s57/s57_reader.hpp"
#include "scene_builder_from_senc.hpp"
#include "portrayal/s52_instruction_ir.hpp"
#include "portrayal/s52_source_catalog_compiler.hpp"
#include "projection/projected_bounds.hpp"
#include "senc/senc_reader.hpp"
#include "senc/senc_writer.hpp"

#include <internal_use_only/config.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace {
using chart_view::runtime::ViewportState;
using chart_view::runtime::catalog::ChartCatalog;
using chart_view::runtime::catalog::ChartSelectionPolicy;
using chart_view::runtime::catalog::CoverageIndex;
using chart_view::runtime::chart_data::Extent;
using chart_view::runtime::chart_data::FeatureChartDataset;
using chart_view::runtime::portrayal::S52ColorScheme;
using chart_view::runtime::portrayal::S52DisplayCategory;
using chart_view::runtime::portrayal::S52DisplaySettings;
using chart_view::runtime::portrayal::S52PointSymbolMode;
using chart_view::runtime::portrayal::S52SourceCatalogCompiler;
using chart_view::runtime::quilt::QuiltLayer;
using chart_view::runtime::quilt::QuiltPlan;

constexpr float kFrameClearR = 230.0F / 255.0F;
constexpr float kFrameClearG = 230.0F / 255.0F;
constexpr float kFrameClearB = 217.0F / 255.0F;
constexpr float kFrameClearA = 1.0F;

constexpr double kMetresPerDegLat = 111320.0;
constexpr double kPixelsPerMetre = 3779.5275591;
constexpr int kDefaultViewportWidth = 1280;
constexpr int kDefaultViewportHeight = 720;
constexpr double kDefaultViewportScale = 100000.0;

constexpr std::uint32_t buildFeatureFlags() noexcept
{
  std::uint32_t flags = 0;

  if(chart_view::build::enable_qt_rhi) {
    flags |= CHART_VIEW_FEATURE_QT_RHI;
  }

  if(chart_view::build::enable_s57) {
    flags |= CHART_VIEW_FEATURE_S57;
  }

  if(chart_view::build::enable_cm93) {
    flags |= CHART_VIEW_FEATURE_CM93;
  }

  if(chart_view::build::enable_s101) {
    flags |= CHART_VIEW_FEATURE_S101;
  }

  return flags;
}

std::string toLowerAscii(std::string value)
{
  std::transform(
    value.begin(),
    value.end(),
    value.begin(),
    [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

std::string toUpperAscii(std::string value)
{
  std::transform(
    value.begin(),
    value.end(),
    value.begin(),
    [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
  return value;
}

chart_view_s52_display_category_t toPublicDisplayCategory(std::string_view value) noexcept
{
  const auto normalized = toLowerAscii(std::string(value));
  if(normalized == "display_base" || normalized == "displaybase" || normalized == "base") {
    return chart_view_s52_display_base;
  }
  if(normalized == "all") {
    return chart_view_s52_display_all;
  }
  return chart_view_s52_display_standard;
}

chart_view_s52_display_category_t toPublicDisplayCategory(S52DisplayCategory category) noexcept
{
  switch(category) {
  case S52DisplayCategory::kDisplayBase:
    return chart_view_s52_display_base;
  case S52DisplayCategory::kAll:
    return chart_view_s52_display_all;
  case S52DisplayCategory::kStandard:
  default:
    return chart_view_s52_display_standard;
  }
}

chart_view_s52_color_palette_t toPublicPalette(S52ColorScheme scheme) noexcept
{
  switch(scheme) {
  case S52ColorScheme::kDusk:
    return chart_view_s52_palette_dusk;
  case S52ColorScheme::kNight:
    return chart_view_s52_palette_night;
  case S52ColorScheme::kDay:
  default:
    return chart_view_s52_palette_day;
  }
}

void exportS52Settings(const S52DisplaySettings &settings, chart_view_s52_mariner_settings_t &out) noexcept
{
  out = {};
  out.palette = toPublicPalette(settings.colorScheme);
  out.display_category = toPublicDisplayCategory(settings.displayCategory);
  out.show_text = settings.showTextLabels ? 1U : 0U;
  out.show_soundings = settings.showSoundings ? 1U : 0U;
  out.simplified_points = settings.pointSymbolMode == S52PointSymbolMode::kSimplified ? 1U : 0U;
  out.two_shades = settings.twoShades ? 1U : 0U;
  out.safety_contour_m = settings.safetyContourMeters;
  out.safety_depth_m = settings.safetyDepthMeters;
  out.shallow_contour_m = settings.shallowContourMeters;
  out.deep_contour_m = settings.deepContourMeters;
  out.shallow_pattern = settings.shallowPattern ? 1U : 0U;
  out.full_sector_lights = settings.fullSectorLights ? 1U : 0U;
  out.symbolized_boundaries = settings.symbolizedBoundaries ? 1U : 0U;
  out.honor_scamin = settings.honorScamin ? 1U : 0U;
}

std::string describeRuleLabel(const chart_view::runtime::portrayal::S52CompiledLookupRow &row)
{
  for(const auto &instruction : row.instructions) {
    if(const auto styleKey = chart_view::runtime::portrayal::instructionStyleKey(instruction);
       !styleKey.empty()) {
      return row.objectAcronym + " -> " + std::string(styleKey);
    }
  }

  return row.objectAcronym;
}

template<typename Entry, typename PublicEntry, typename NameAccessor>
chart_view_status_t exportFilterEntries(
  const std::vector<Entry> &entries,
  PublicEntry *out,
  std::uint32_t &inoutCount,
  NameAccessor accessor)
{
  const auto required = static_cast<std::uint32_t>(entries.size());
  if(out == nullptr) {
    inoutCount = required;
    return chart_view_status_ok;
  }

  if(inoutCount < required) {
    inoutCount = required;
    return chart_view_status_invalid_argument;
  }

  for(std::uint32_t index = 0; index < required; ++index) {
    out[index] = {};
    accessor(entries[index], out[index]);
  }
  inoutCount = required;
  return chart_view_status_ok;
}

chart_view_chart_source_type_t detectSourceTypeFromPath(std::string_view path)
{
  if(path.empty()) {
    return chart_view_chart_source_unknown;
  }

  std::error_code ec;
  if(std::filesystem::is_directory(std::filesystem::path(path), ec)) {
    return chart_view_chart_source_cm93;
  }

  auto ext = toLowerAscii(std::filesystem::path(path).extension().string());
  if(!ext.empty() && ext.front() == '.') {
    ext.erase(ext.begin());
  }

  if(ext == "c" || ext == "cm93") {
    return chart_view_chart_source_cm93;
  }

  if(ext == "s101" || ext == "101" || ext == "gml" || ext == "xml") {
    return chart_view_chart_source_s101;
  }

  if(ext == "000" || ext == "001" || ext == "002" || ext == "s57") {
    return chart_view_chart_source_s57;
  }

  return chart_view_chart_source_unknown;
}

chart_view_chart_source_type_t detectSourceTypeFromExtension(const std::filesystem::path &path)
{
  auto ext = toLowerAscii(path.extension().string());
  if(ext == ".s101" || ext == ".101" || ext == ".gml" || ext == ".xml") {
    return chart_view_chart_source_s101;
  }

  if(ext == ".000" || ext == ".001" || ext == ".002" || ext == ".s57") {
    return chart_view_chart_source_s57;
  }

  return chart_view_chart_source_unknown;
}

double metresPerDegreeLon(double centerLat) noexcept
{
  const auto cosLat = std::cos(centerLat * 3.14159265358979323846 / 180.0);
  return kMetresPerDegLat * (cosLat > 1e-6 ? cosLat : 1e-6);
}

double estimateScaleForExtent(const Extent &extent, int pixelWidth, int pixelHeight) noexcept
{
  if(!extent.isValid()) {
    return kDefaultViewportScale;
  }

  const auto centerLat = (extent.minLat + extent.maxLat) * 0.5;
  const auto widthMeters =
    std::max(0.0, extent.maxLon - extent.minLon) * metresPerDegreeLon(centerLat);
  const auto heightMeters =
    std::max(0.0, extent.maxLat - extent.minLat) * kMetresPerDegLat;

  const auto safeWidth = std::max(pixelWidth, 1);
  const auto safeHeight = std::max(pixelHeight, 1);
  const auto scaleWidth = widthMeters * kPixelsPerMetre / static_cast<double>(safeWidth);
  const auto scaleHeight = heightMeters * kPixelsPerMetre / static_cast<double>(safeHeight);
  const auto fittedScale = std::max(scaleWidth, scaleHeight);
  return std::max(fittedScale * 1.1, 1000.0);
}

Extent invalidExtent() noexcept
{
  return {1.0, 1.0, 0.0, 0.0};
}

Extent computeViewportExtent(const chart_view_viewport_t &vp) noexcept
{
  if(vp.pixel_width <= 0 || vp.pixel_height <= 0 || vp.scale_denominator <= 0.0) {
    return invalidExtent();
  }

  const auto projectionContext = chart_view::runtime::projection::ProjectionContext::createMercator();
  if(!projectionContext.isValid()) {
    return invalidExtent();
  }

  Extent extent{};
  if(!chart_view::runtime::projection::computeViewportGeographicExtent(
       vp,
       projectionContext,
       extent)) {
    return invalidExtent();
  }

  return extent;
}

Extent unionExtents(const Extent &lhs, const Extent &rhs) noexcept
{
  if(!lhs.isValid()) {
    return rhs;
  }
  if(!rhs.isValid()) {
    return lhs;
  }

  return {
    std::min(lhs.minLon, rhs.minLon),
    std::min(lhs.minLat, rhs.minLat),
    std::max(lhs.maxLon, rhs.maxLon),
    std::max(lhs.maxLat, rhs.maxLat)};
}

struct QueryLocalPoint
{
  double x{0.0};
  double y{0.0};
};

QueryLocalPoint toLocalPoint(const chart_view::runtime::chart_data::Coordinate &coordinate, double referenceLat) noexcept
{
  return {
    coordinate.lon * metresPerDegreeLon(referenceLat),
    coordinate.lat * kMetresPerDegLat};
}

double squaredDistance(const QueryLocalPoint &lhs, const QueryLocalPoint &rhs) noexcept
{
  const auto dx = lhs.x - rhs.x;
  const auto dy = lhs.y - rhs.y;
  return dx * dx + dy * dy;
}

double distancePointToSegmentMeters(
  const chart_view::runtime::chart_data::Coordinate &point,
  const chart_view::runtime::chart_data::Coordinate &start,
  const chart_view::runtime::chart_data::Coordinate &end,
  double referenceLat) noexcept
{
  const auto p = toLocalPoint(point, referenceLat);
  const auto a = toLocalPoint(start, referenceLat);
  const auto b = toLocalPoint(end, referenceLat);
  const auto abx = b.x - a.x;
  const auto aby = b.y - a.y;
  const auto abLengthSq = abx * abx + aby * aby;
  if(abLengthSq <= 1e-12) {
    return std::sqrt(squaredDistance(p, a));
  }

  const auto apx = p.x - a.x;
  const auto apy = p.y - a.y;
  const auto rawT = (apx * abx + apy * aby) / abLengthSq;
  const auto t = std::clamp(rawT, 0.0, 1.0);
  QueryLocalPoint projected{
    a.x + abx * t,
    a.y + aby * t};
  return std::sqrt(squaredDistance(p, projected));
}

Extent computeFeatureExtent(const chart_view::runtime::chart_data::Feature &feature) noexcept
{
  Extent extent = invalidExtent();
  auto include = [&](const chart_view::runtime::chart_data::Coordinate &vertex) {
    if(!std::isfinite(vertex.lon) || !std::isfinite(vertex.lat)) {
      return;
    }

    if(!extent.isValid()) {
      extent = {vertex.lon, vertex.lat, vertex.lon, vertex.lat};
      return;
    }

    extent.minLon = std::min(extent.minLon, vertex.lon);
    extent.minLat = std::min(extent.minLat, vertex.lat);
    extent.maxLon = std::max(extent.maxLon, vertex.lon);
    extent.maxLat = std::max(extent.maxLat, vertex.lat);
  };

  std::visit(
    [&](auto &&geometry) {
      using T = std::decay_t<decltype(geometry)>;

      if constexpr(std::is_same_v<T, chart_view::runtime::chart_data::PointGeometry>) {
        include(geometry.position);
      } else if constexpr(std::is_same_v<T, chart_view::runtime::chart_data::LineGeometry>) {
        for(const auto &vertex : geometry.vertices) {
          include(vertex);
        }
      } else if constexpr(std::is_same_v<T, chart_view::runtime::chart_data::AreaGeometry>) {
        for(const auto &vertex : geometry.exteriorRing) {
          include(vertex);
        }
        for(const auto &ring : geometry.interiorRings) {
          for(const auto &vertex : ring) {
            include(vertex);
          }
        }
      }
    },
    feature.geometry);

  return extent;
}

bool extentCouldContainQuery(
  const Extent &extent,
  const chart_view::runtime::chart_data::Coordinate &query,
  double toleranceMeters) noexcept
{
  if(!extent.isValid()) {
    return false;
  }

  const auto lonTolerance = toleranceMeters / metresPerDegreeLon(query.lat);
  const auto latTolerance = toleranceMeters / kMetresPerDegLat;
  return query.lon >= extent.minLon - lonTolerance && query.lon <= extent.maxLon + lonTolerance
      && query.lat >= extent.minLat - latTolerance && query.lat <= extent.maxLat + latTolerance;
}

bool pointInRing(
  const chart_view::runtime::chart_data::Coordinate &point,
  const std::vector<chart_view::runtime::chart_data::Coordinate> &ring) noexcept
{
  if(ring.size() < 3U) {
    return false;
  }

  bool inside = false;
  for(std::size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++) {
    const auto &a = ring[i];
    const auto &b = ring[j];
    const bool intersects = ((a.lat > point.lat) != (b.lat > point.lat))
      && (point.lon
          < (b.lon - a.lon) * (point.lat - a.lat) / ((b.lat - a.lat) == 0.0 ? 1e-12 : (b.lat - a.lat))
                + a.lon);
    if(intersects) {
      inside = !inside;
    }
  }

  return inside;
}

bool areaContainsPoint(
  const chart_view::runtime::chart_data::AreaGeometry &geometry,
  const chart_view::runtime::chart_data::Coordinate &point) noexcept
{
  if(!pointInRing(point, geometry.exteriorRing)) {
    return false;
  }

  return std::none_of(
    geometry.interiorRings.begin(),
    geometry.interiorRings.end(),
    [&](const auto &ring) { return pointInRing(point, ring); });
}

double minimumBoundaryDistanceMeters(
  const std::vector<chart_view::runtime::chart_data::Coordinate> &ring,
  const chart_view::runtime::chart_data::Coordinate &query,
  double referenceLat) noexcept
{
  if(ring.size() < 2U) {
    return std::numeric_limits<double>::infinity();
  }

  double best = std::numeric_limits<double>::infinity();
  for(std::size_t i = 1; i < ring.size(); ++i) {
    best = std::min(best, distancePointToSegmentMeters(query, ring[i - 1], ring[i], referenceLat));
  }
  best = std::min(best, distancePointToSegmentMeters(query, ring.back(), ring.front(), referenceLat));
  return best;
}

std::optional<double> hitDistanceMeters(
  const chart_view::runtime::chart_data::Feature &feature,
  const chart_view::runtime::chart_data::Coordinate &query,
  double toleranceMeters) noexcept
{
  const auto extent = computeFeatureExtent(feature);
  if(!extentCouldContainQuery(extent, query, toleranceMeters)) {
    return std::nullopt;
  }

  const auto referenceLat = query.lat;
  std::optional<double> hit;

  std::visit(
    [&](auto &&geometry) {
      using T = std::decay_t<decltype(geometry)>;

      if constexpr(std::is_same_v<T, chart_view::runtime::chart_data::PointGeometry>) {
        const auto distance = std::sqrt(
          squaredDistance(toLocalPoint(query, referenceLat), toLocalPoint(geometry.position, referenceLat)));
        if(distance <= toleranceMeters) {
          hit = distance;
        }
      } else if constexpr(std::is_same_v<T, chart_view::runtime::chart_data::LineGeometry>) {
        if(geometry.vertices.size() < 2U) {
          return;
        }

        double best = std::numeric_limits<double>::infinity();
        for(std::size_t index = 1; index < geometry.vertices.size(); ++index) {
          best = std::min(
            best,
            distancePointToSegmentMeters(
              query,
              geometry.vertices[index - 1],
              geometry.vertices[index],
              referenceLat));
        }
        if(best <= toleranceMeters) {
          hit = best;
        }
      } else if constexpr(std::is_same_v<T, chart_view::runtime::chart_data::AreaGeometry>) {
        if(areaContainsPoint(geometry, query)) {
          hit = 0.0;
          return;
        }

        double best = minimumBoundaryDistanceMeters(geometry.exteriorRing, query, referenceLat);
        for(const auto &ring : geometry.interiorRings) {
          best = std::min(best, minimumBoundaryDistanceMeters(ring, query, referenceLat));
        }
        if(best <= toleranceMeters) {
          hit = best;
        }
      }
    },
    feature.geometry);

  return hit;
}

struct SelectedFeatureName
{
  std::string value;
  std::string sourceAttribute;
};

std::optional<SelectedFeatureName> selectPrimaryFeatureName(const chart_view::runtime::chart_data::Feature &feature)
{
  for(const auto attribute : {std::string_view("NOBJNM"), std::string_view("OBJNAM")}) {
    const auto *value = chart_view::runtime::label::findStringAttribute(feature, attribute);
    if(value != nullptr && !value->empty()) {
      return SelectedFeatureName{*value, std::string(attribute)};
    }
  }

  return std::nullopt;
}

void setViewportFromExtent(ViewportState &viewport, const Extent &extent)
{
  auto vp = viewport.viewport();
  if(vp.pixel_width <= 0) {
    vp.pixel_width = kDefaultViewportWidth;
  }
  if(vp.pixel_height <= 0) {
    vp.pixel_height = kDefaultViewportHeight;
  }

  if(extent.isValid()) {
    vp.center_lon = (extent.minLon + extent.maxLon) * 0.5;
    vp.center_lat = (extent.minLat + extent.maxLat) * 0.5;
    vp.scale_denominator = estimateScaleForExtent(extent, vp.pixel_width, vp.pixel_height);
  } else if(vp.scale_denominator <= 0.0) {
    vp.scale_denominator = kDefaultViewportScale;
  }

  viewport.set(vp);
}

void setViewportFromDataset(ViewportState &viewport, const FeatureChartDataset &dataset)
{
  auto vp = viewport.viewport();
  if(vp.pixel_width <= 0) {
    vp.pixel_width = kDefaultViewportWidth;
  }
  if(vp.pixel_height <= 0) {
    vp.pixel_height = kDefaultViewportHeight;
  }
  if(vp.scale_denominator <= 0.0) {
    vp.scale_denominator = kDefaultViewportScale;
  }

  const auto &extent = dataset.meta().extent;
  if(extent.isValid()) {
    vp.center_lon = (extent.minLon + extent.maxLon) * 0.5;
    vp.center_lat = (extent.minLat + extent.maxLat) * 0.5;
  }

  viewport.set(vp);
}

bool looksLikeSencDirectory(const std::filesystem::path &directory)
{
  std::error_code ec;
  for(const auto &entry : std::filesystem::directory_iterator(directory, ec)) {
    if(ec) {
      return false;
    }

    if(entry.is_regular_file(ec) && !ec &&
       toLowerAscii(entry.path().extension().string()) == ".senc") {
      return true;
    }
  }

  return false;
}

std::vector<std::pair<std::filesystem::path, chart_view_chart_source_type_t>> collectSourceChartFiles(
  const std::filesystem::path &directory)
{
  std::vector<std::pair<std::filesystem::path, chart_view_chart_source_type_t>> files;

  std::error_code ec;
  for(const auto &entry : std::filesystem::recursive_directory_iterator(directory, ec)) {
    if(ec) {
      files.clear();
      return files;
    }

    if(!entry.is_regular_file(ec) || ec) {
      continue;
    }

    const auto sourceType = detectSourceTypeFromExtension(entry.path());
    if(sourceType == chart_view_chart_source_unknown || sourceType == chart_view_chart_source_cm93) {
      continue;
    }

    files.emplace_back(entry.path(), sourceType);
  }

  std::sort(
    files.begin(),
    files.end(),
    [](const auto &lhs, const auto &rhs) {
      return lhs.first.generic_string() < rhs.first.generic_string();
    });
  return files;
}

bool writeBlobFile(const std::filesystem::path &path, std::span<const std::uint8_t> blob)
{
  std::ofstream stream(path, std::ios::binary | std::ios::trunc);
  if(!stream.is_open()) {
    return false;
  }

  stream.write(reinterpret_cast<const char *>(blob.data()), static_cast<std::streamsize>(blob.size()));
  return stream.good();
}

bool loadSourceDataset(const std::string &path,
                       chart_view_chart_source_type_t sourceType,
                       FeatureChartDataset &out)
{
  switch(sourceType) {
  case chart_view_chart_source_s57: {
    chart_view::runtime::s57::S57Reader reader;
    const auto result = reader.read(path);
    if(!result.ok) {
      return false;
    }
    out = std::move(result.dataset);
    return true;
  }
  case chart_view_chart_source_cm93: {
    chart_view::runtime::cm93::Cm93Reader reader;
    std::error_code ec;
    const auto result =
      std::filesystem::is_directory(path, ec) ? reader.readFirstCell(path) : reader.read(path);
    if(!result.ok) {
      std::fprintf(stderr, "cm93_read_failed: %s\n", result.error.c_str());
      return false;
    }
    out = std::move(result.dataset);
    return true;
  }
  case chart_view_chart_source_s101: {
    chart_view::runtime::s101::S101Reader reader;
    const auto result = reader.read(path);
    if(!result.ok) {
      return false;
    }
    out = std::move(result.dataset);
    return true;
  }
  case chart_view_chart_source_unknown:
  default:
    return false;
  }
}

bool buildSencCatalogFromSourceDirectory(const std::filesystem::path &sourceDirectory,
                                         const std::filesystem::path &catalogDirectory)
{
  const auto sources = collectSourceChartFiles(sourceDirectory);
  if(sources.empty()) {
    return false;
  }

  std::error_code ec;
  std::filesystem::create_directories(catalogDirectory, ec);
  if(ec) {
    return false;
  }

  chart_view::runtime::senc::SencWriter writer;
  std::unordered_set<std::string> usedNames;
  bool wroteAny = false;

  for(const auto &[sourcePath, sourceType] : sources) {
    FeatureChartDataset dataset;
    if(!loadSourceDataset(sourcePath.string(), sourceType, dataset)) {
      continue;
    }

    auto meta = dataset.meta();
    if(meta.sourceType == chart_view_chart_source_unknown) {
      meta.sourceType = sourceType;
    }

    if(dataset.empty() || !meta.extent.isValid()) {
      continue;
    }

    std::string baseName = meta.name.empty() ? sourcePath.stem().string() : meta.name;
    if(baseName.empty()) {
      baseName = "chart";
    }

    std::string uniqueName = baseName;
    for(std::uint32_t suffix = 2; !usedNames.insert(uniqueName).second; ++suffix) {
      uniqueName = baseName + "_" + std::to_string(suffix);
    }

    meta.name = uniqueName;
    dataset.setMeta(std::move(meta));

    const auto blob = writer.write(dataset);
    if(blob.empty()) {
      continue;
    }

    if(!writeBlobFile(catalogDirectory / (uniqueName + ".senc"), blob)) {
      continue;
    }

    wroteAny = true;
  }

  return wroteAny;
}

Extent computeCatalogExtent(const ChartCatalog &catalog)
{
  Extent extent = invalidExtent();
  for(const auto &entry : catalog.entries()) {
    extent = unionExtents(extent, entry.extent);
  }
  return extent;
}

bool loadPlanDatasets(const QuiltPlan &plan, std::vector<FeatureChartDataset> &datasets)
{
  chart_view::runtime::senc::SencReader reader;
  datasets.clear();
  datasets.reserve(plan.layers().size());

  for(const auto &layer : plan.layers()) {
    const auto result = reader.readFromFile(layer.sencPath.string());
    if(!result.ok) {
      datasets.clear();
      return false;
    }
    datasets.push_back(std::move(result.dataset));
  }

  return true;
}

chart_view_zoom_scale_state_t toZoomScaleState(
  chart_view::runtime::quilt::ZoomScaleState state) noexcept
{
  switch(state) {
  case chart_view::runtime::quilt::ZoomScaleState::kNormal:
    return chart_view_zoom_scale_normal;
  case chart_view::runtime::quilt::ZoomScaleState::kOverzoom:
    return chart_view_zoom_scale_overzoom;
  case chart_view::runtime::quilt::ZoomScaleState::kUnderzoom:
    return chart_view_zoom_scale_underzoom;
  case chart_view::runtime::quilt::ZoomScaleState::kNoCharts:
  default:
    return chart_view_zoom_scale_no_charts;
  }
}

chart_view_zoom_rebuild_reason_t toZoomRebuildReason(
  chart_view::runtime::quilt::ZoomRebuildReason reason) noexcept
{
  switch(reason) {
  case chart_view::runtime::quilt::ZoomRebuildReason::kEmptyPlan:
    return chart_view_zoom_rebuild_empty_plan;
  case chart_view::runtime::quilt::ZoomRebuildReason::kPreferredChartChanged:
    return chart_view_zoom_rebuild_preferred_chart_changed;
  case chart_view::runtime::quilt::ZoomRebuildReason::kOverzoom:
    return chart_view_zoom_rebuild_overzoom;
  case chart_view::runtime::quilt::ZoomRebuildReason::kUnderzoom:
    return chart_view_zoom_rebuild_underzoom;
  case chart_view::runtime::quilt::ZoomRebuildReason::kNone:
  default:
    return chart_view_zoom_rebuild_none;
  }
}

}// namespace

namespace chart_view::runtime {

RuntimeContext::RuntimeContext()
  : m_info{
      CHART_VIEW_RUNTIME_ABI_VERSION,
      chart_view::build::project_name.data(),
      chart_view::build::project_version.data(),
      chart_view::build::git_sha.data(),
      buildFeatureFlags()}
{
}

RuntimeContext::~RuntimeContext()
{
  if(m_state == RuntimeState::kInitialized) {
    shutdown();
  } else {
    clearCatalogCache();
  }
}

chart_view_status_t RuntimeContext::initialize()
{
  if(m_state == RuntimeState::kInitialized) {
    return chart_view_status_already_initialized;
  }

  m_state = RuntimeState::kInitialized;
  return chart_view_status_ok;
}

chart_view_status_t RuntimeContext::shutdown()
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  clearLoadedCharts();
  m_state = RuntimeState::kShutDown;
  return chart_view_status_ok;
}

void RuntimeContext::clearCatalogCache()
{
  if(m_catalogCacheDirectory.empty()) {
    return;
  }

  std::error_code ec;
  std::filesystem::remove_all(m_catalogCacheDirectory, ec);
  m_catalogCacheDirectory.clear();
}

void RuntimeContext::clearLoadedCharts()
{
  m_dataset.reset();
  m_quiltDatasets.clear();
  m_quiltPlan.clear();
  m_catalog.clear();
  m_coverageIndex.clear();
  m_directoryMode = false;
  m_featureQueryCache.clear();
  m_featureDescribeCache = {};
  clearCatalogCache();
}

chart_view_status_t RuntimeContext::ensureRenderTarget()
{
  if(!m_viewport.isValid()) {
    return chart_view_status_invalid_argument;
  }

  const auto &vp = m_viewport.viewport();
  return m_renderBackend.ensureSurfaceSize(vp.pixel_width, vp.pixel_height);
}

chart_view_status_t RuntimeContext::rebuildDirectoryPlan()
{
  if(!m_directoryMode) {
    return chart_view_status_ok;
  }

  if(m_catalog.empty() || !m_viewport.isValid()) {
    m_quiltPlan.clear();
    m_quiltDatasets.clear();
    return chart_view_status_ok;
  }

  quilt::QuiltPlanner planner;
  const auto plan = planner.build(
    m_coverageIndex,
    m_selectionPolicy,
    computeViewportExtent(m_viewport.viewport()),
    m_viewport.viewport().scale_denominator);

  if(plan.empty()) {
    m_quiltPlan.clear();
    m_quiltDatasets.clear();
    return chart_view_status_ok;
  }

  std::vector<FeatureChartDataset> datasets;
  if(!loadPlanDatasets(plan, datasets)) {
    return chart_view_status_invalid_format;
  }

  m_quiltPlan = plan;
  m_quiltDatasets = std::move(datasets);
  return chart_view_status_ok;
}

chart_view_status_t RuntimeContext::setViewport(const chart_view_viewport_t &vp)
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  m_viewport.set(vp);
  m_featureQueryCache.clear();
  m_featureDescribeCache = {};

  if(m_directoryMode) {
    const auto rebuildStatus = rebuildDirectoryPlan();
    if(rebuildStatus != chart_view_status_ok) {
      return rebuildStatus;
    }
  }

  if(m_viewport.isValid()) {
    const auto targetStatus = ensureRenderTarget();
    if(targetStatus != chart_view_status_ok) {
      return targetStatus;
    }

    const auto clearStatus =
      m_renderBackend.renderClearFrame(kFrameClearR, kFrameClearG, kFrameClearB, kFrameClearA);
    if(clearStatus != chart_view_status_ok) {
      return clearStatus;
    }
  }

  return chart_view_status_ok;
}

chart_view_status_t RuntimeContext::stepZoom(std::int32_t stepCount, chart_view_zoom_result_t &out)
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  out = {};
  if(!m_viewport.isValid()) {
    return chart_view_status_invalid_argument;
  }

  quilt::QuiltPlan currentPlan;
  currentPlan.setViewport(
    computeViewportExtent(m_viewport.viewport()),
    m_viewport.viewport().scale_denominator);

  if(m_directoryMode) {
    currentPlan = m_quiltPlan;
  } else if(m_dataset) {
    const auto &meta = m_dataset->meta();
    QuiltLayer layer;
    layer.chartId = meta.name;
    layer.sourceType = meta.sourceType;
    layer.nativeScale = meta.nativeScale;
    layer.usageBand = meta.usageBand;
    layer.fullExtent = meta.extent;
    layer.visibleExtent = meta.extent;
    currentPlan.addLayer(std::move(layer));
  }

  quilt::ZoomPolicy zoomPolicy;
  const auto steps =
    stepCount >= 0 ? static_cast<std::uint32_t>(stepCount)
                   : static_cast<std::uint32_t>(-static_cast<std::int64_t>(stepCount));
  const auto requestedScale = stepCount >= 0
                                ? zoomPolicy.zoomIn(m_viewport.viewport().scale_denominator, steps)
                                : zoomPolicy.zoomOut(m_viewport.viewport().scale_denominator, steps);
  const auto decision = zoomPolicy.evaluate(currentPlan, requestedScale);

  auto vp = m_viewport.viewport();
  vp.scale_denominator = decision.resolvedScaleDenominator > 0.0
                           ? decision.resolvedScaleDenominator
                           : m_viewport.viewport().scale_denominator;

  const auto setStatus = setViewport(vp);
  if(setStatus != chart_view_status_ok) {
    return setStatus;
  }

  out.viewport = m_viewport.viewport();
  out.requested_scale_denominator = decision.requestedScaleDenominator;
  out.resolved_scale_denominator = decision.resolvedScaleDenominator;
  out.preferred_layer_index = static_cast<std::uint32_t>(decision.preferredLayerIndex);
  out.primary_scale_state = toZoomScaleState(decision.primaryScaleState);
  out.should_rebuild_plan = decision.shouldRebuildPlan ? 1U : 0U;
  out.rebuild_reason = toZoomRebuildReason(decision.rebuildReason);
  return chart_view_status_ok;
}

chart_view_status_t RuntimeContext::loadSenc(std::span<const std::uint8_t> data)
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  if(data.empty()) {
    return chart_view_status_invalid_argument;
  }

  senc::SencReader reader;
  auto result = reader.read(data);
  if(!result.ok) {
    return chart_view_status_invalid_format;
  }

  clearLoadedCharts();
  m_dataset = std::make_unique<FeatureChartDataset>(std::move(result.dataset));
  return chart_view_status_ok;
}

chart_view_status_t RuntimeContext::openChartFile(std::string_view path,
                                                  chart_view_chart_source_type_t sourceType)
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  if(path.empty()) {
    return chart_view_status_invalid_argument;
  }

  const auto resolvedType =
    sourceType == chart_view_chart_source_unknown ? detectSourceTypeFromPath(path) : sourceType;
  if(resolvedType == chart_view_chart_source_unknown) {
    return chart_view_status_invalid_argument;
  }

  FeatureChartDataset dataset;
  if(!loadSourceDataset(std::string(path), resolvedType, dataset)) {
    return chart_view_status_invalid_format;
  }

  senc::SencWriter writer;
  const auto sencBlob = writer.write(dataset);
  if(sencBlob.empty()) {
    return chart_view_status_invalid_format;
  }

  auto newViewport = m_viewport;
  setViewportFromDataset(newViewport, dataset);

  const auto previousViewport = m_viewport;
  m_viewport = newViewport;
  const auto loadStatus = loadSenc(std::span<const std::uint8_t>(sencBlob.data(), sencBlob.size()));
  if(loadStatus != chart_view_status_ok) {
    m_viewport = previousViewport;
    return loadStatus;
  }

  return chart_view_status_ok;
}

chart_view_status_t RuntimeContext::openChartDirectory(std::string_view path)
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  if(path.empty()) {
    return chart_view_status_invalid_argument;
  }

  const std::filesystem::path directory(path);
  std::error_code ec;
  if(!std::filesystem::exists(directory, ec) || ec || !std::filesystem::is_directory(directory, ec) ||
     ec) {
    return chart_view_status_invalid_argument;
  }

  std::filesystem::path catalogDirectory = directory;
  std::filesystem::path cacheDirectory;

  if(!looksLikeSencDirectory(directory)) {
    cacheDirectory = std::filesystem::temp_directory_path() /
                     ("chart_view_catalog_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
    std::filesystem::remove_all(cacheDirectory, ec);
    ec.clear();
    std::filesystem::create_directories(cacheDirectory, ec);
    if(ec || !buildSencCatalogFromSourceDirectory(directory, cacheDirectory)) {
      std::filesystem::remove_all(cacheDirectory, ec);
      return chart_view_status_invalid_format;
    }
    catalogDirectory = cacheDirectory;
  }

  ChartCatalog catalog;
  if(!catalog.loadDirectory(catalogDirectory) || catalog.empty()) {
    std::filesystem::remove_all(cacheDirectory, ec);
    return chart_view_status_invalid_format;
  }

  CoverageIndex coverageIndex;
  if(!coverageIndex.build(catalog)) {
    std::filesystem::remove_all(cacheDirectory, ec);
    return chart_view_status_invalid_format;
  }

  const auto catalogExtent = computeCatalogExtent(catalog);
  if(!catalogExtent.isValid()) {
    std::filesystem::remove_all(cacheDirectory, ec);
    return chart_view_status_invalid_format;
  }

  auto newViewport = m_viewport;
  setViewportFromExtent(newViewport, catalogExtent);

  quilt::QuiltPlanner planner;
  const auto initialPlan = planner.build(
    coverageIndex,
    m_selectionPolicy,
    computeViewportExtent(newViewport.viewport()),
    newViewport.viewport().scale_denominator);
  if(initialPlan.empty()) {
    std::filesystem::remove_all(cacheDirectory, ec);
    return chart_view_status_invalid_format;
  }

  std::vector<FeatureChartDataset> datasets;
  if(!loadPlanDatasets(initialPlan, datasets) || datasets.empty()) {
    std::filesystem::remove_all(cacheDirectory, ec);
    return chart_view_status_invalid_format;
  }

  clearLoadedCharts();
  m_catalog = std::move(catalog);
  m_coverageIndex = std::move(coverageIndex);
  m_catalogCacheDirectory = std::move(cacheDirectory);
  m_directoryMode = true;
  m_quiltPlan = initialPlan;
  m_quiltDatasets = std::move(datasets);
  m_viewport = newViewport;

  const auto targetStatus = ensureRenderTarget();
  if(targetStatus != chart_view_status_ok) {
    return targetStatus;
  }

  const auto clearStatus =
    m_renderBackend.renderClearFrame(kFrameClearR, kFrameClearG, kFrameClearB, kFrameClearA);
  if(clearStatus != chart_view_status_ok) {
    return clearStatus;
  }

  return chart_view_status_ok;
}

chart_view_status_t RuntimeContext::setS52MarinerSettings(const S52DisplaySettings &settings)
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  m_s52Settings = settings;
  m_renderer.setS52Settings(m_s52Settings);
  m_featureQueryCache.clear();
  m_featureDescribeCache = {};
  return chart_view_status_ok;
}

void RuntimeContext::getS52MarinerSettings(chart_view_s52_mariner_settings_t &out) const
{
  exportS52Settings(m_s52Settings, out);
}

chart_view_status_t RuntimeContext::setS57ClassFilters(
  std::span<const chart_view_s57_class_filter_t> filters)
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  m_s57ClassFilters.clear();
  m_s57ClassFilters.reserve(filters.size());
  for(const auto &filter : filters) {
    if(filter.object_acronym == nullptr || filter.object_acronym[0] == '\0') {
      return chart_view_status_invalid_argument;
    }

    m_s57ClassFilters.push_back({toUpperAscii(filter.object_acronym), filter.enabled != 0U});
  }

  std::vector<portrayal::S57ClassSelectionFilter> rendererFilters;
  rendererFilters.reserve(m_s57ClassFilters.size());
  for(const auto &entry : m_s57ClassFilters) {
    rendererFilters.push_back({entry.objectAcronym, entry.enabled});
  }
  m_renderer.setS57ClassFilters(rendererFilters);
  m_featureQueryCache.clear();
  m_featureDescribeCache = {};

  return chart_view_status_ok;
}

chart_view_status_t RuntimeContext::getS57ClassFilters(
  chart_view_s57_class_filter_t *out,
  std::uint32_t &inoutCount) const
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  return exportFilterEntries(
    m_s57ClassFilters,
    out,
    inoutCount,
    [](const S57ClassFilterEntry &entry, chart_view_s57_class_filter_t &dto) {
      dto.object_acronym = entry.objectAcronym.c_str();
      dto.enabled = entry.enabled ? 1U : 0U;
    });
}

chart_view_status_t RuntimeContext::setS52RuleFilters(
  std::span<const chart_view_s52_rule_filter_t> filters)
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  m_s52RuleFilters.clear();
  m_s52RuleFilters.reserve(filters.size());
  for(const auto &filter : filters) {
    if(filter.rule_id == nullptr || filter.rule_id[0] == '\0') {
      return chart_view_status_invalid_argument;
    }

    m_s52RuleFilters.push_back({toLowerAscii(filter.rule_id), filter.enabled != 0U});
  }

  std::vector<portrayal::S52RuleSelectionFilter> rendererFilters;
  rendererFilters.reserve(m_s52RuleFilters.size());
  for(const auto &entry : m_s52RuleFilters) {
    rendererFilters.push_back({entry.ruleId, entry.enabled});
  }
  m_renderer.setS52RuleFilters(rendererFilters);
  m_featureQueryCache.clear();
  m_featureDescribeCache = {};

  return chart_view_status_ok;
}

chart_view_status_t RuntimeContext::getS52RuleFilters(
  chart_view_s52_rule_filter_t *out,
  std::uint32_t &inoutCount) const
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  return exportFilterEntries(
    m_s52RuleFilters,
    out,
    inoutCount,
    [](const S52RuleFilterEntry &entry, chart_view_s52_rule_filter_t &dto) {
      dto.rule_id = entry.ruleId.c_str();
      dto.enabled = entry.enabled ? 1U : 0U;
    });
}

chart_view_status_t RuntimeContext::enumerateS52Rules(
  chart_view_s52_rule_descriptor_t *out,
  std::uint32_t &inoutCount) const
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  const auto &descriptors = compiledRuleDescriptors();
  const auto required = static_cast<std::uint32_t>(descriptors.size());
  if(out == nullptr) {
    inoutCount = required;
    return chart_view_status_ok;
  }

  if(inoutCount < required) {
    inoutCount = required;
    return chart_view_status_invalid_argument;
  }

  for(std::uint32_t index = 0; index < required; ++index) {
    out[index] = {};
    out[index].rule_id = descriptors[index].ruleId.c_str();
    out[index].object_acronym = descriptors[index].objectAcronym.c_str();
    out[index].view_group = descriptors[index].viewGroup;
    out[index].display_category = descriptors[index].displayCategory;
    out[index].label = descriptors[index].label.c_str();
  }
  inoutCount = required;
  return chart_view_status_ok;
}

const std::vector<RuntimeContext::S52RuleDescriptorEntry> &RuntimeContext::compiledRuleDescriptors() const
{
  static const auto descriptors = [] {
    std::vector<S52RuleDescriptorEntry> entries;
    const auto catalog = S52SourceCatalogCompiler::compileBuiltin();
    entries.reserve(catalog.lookupRows.size());
    for(const auto &row : catalog.lookupRows) {
      entries.push_back(
        {row.ruleId,
         row.objectAcronym,
         row.viewGroup,
         toPublicDisplayCategory(row.displayCategory),
         describeRuleLabel(row)});
    }
    return entries;
  }();
  return descriptors;
}

chart_view_status_t RuntimeContext::queryFeaturesAtPoint(
  const chart_view_feature_query_t &query,
  chart_view_feature_summary_t *out,
  std::uint32_t &inoutCount) const
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  if(!std::isfinite(query.lon) || !std::isfinite(query.lat) || !std::isfinite(query.tolerance_m)
     || query.tolerance_m < 0.0) {
    return chart_view_status_invalid_argument;
  }

  const chart_data::Coordinate hitPoint{query.lon, query.lat};
  const auto toleranceMeters = query.tolerance_m > 0.0 ? query.tolerance_m : 50.0;
  auto symbolizerSettings = m_s52Settings;
  symbolizerSettings.viewingScaleDenominator = m_viewport.viewport().scale_denominator;
  portrayal::FeatureSymbolizer symbolizer(symbolizerSettings);
  if(!m_s57ClassFilters.empty()) {
    std::vector<portrayal::S57ClassSelectionFilter> classFilters;
    classFilters.reserve(m_s57ClassFilters.size());
    for(const auto &entry : m_s57ClassFilters) {
      classFilters.push_back({entry.objectAcronym, entry.enabled});
    }
    symbolizer.setS57ClassFilters(classFilters);
  }
  if(!m_s52RuleFilters.empty()) {
    std::vector<portrayal::S52RuleSelectionFilter> ruleFilters;
    ruleFilters.reserve(m_s52RuleFilters.size());
    for(const auto &entry : m_s52RuleFilters) {
      ruleFilters.push_back({entry.ruleId, entry.enabled});
    }
    symbolizer.setS52RuleFilters(ruleFilters);
  }
  const auto &ruleDescriptors = compiledRuleDescriptors();

  auto describeEntry = [&](FeatureSummaryEntry &entry,
                           const chart_data::FeatureChartDataset &dataset,
                           const chart_data::Feature &feature,
                           std::uint32_t runtimeFeatureToken,
                           double hitDistanceMetersValue) {
    entry = {};
    entry.runtimeFeatureToken = runtimeFeatureToken;
    entry.featureId = feature.id;
    entry.sourceType = dataset.meta().sourceType;
    entry.datasetName = dataset.meta().name;
    entry.classCode = feature.classCode;
    entry.objectAcronym = feature.classAcronym;
    switch(chart_data::geometryType(feature.geometry)) {
    case chart_data::GeometryType::kLine:
      entry.geometryType = chart_view_feature_geometry_line;
      break;
    case chart_data::GeometryType::kArea:
      entry.geometryType = chart_view_feature_geometry_area;
      break;
    case chart_data::GeometryType::kPoint:
    default:
      entry.geometryType = chart_view_feature_geometry_point;
      break;
    }
    entry.extent = computeFeatureExtent(feature);
    entry.hitDistanceMeters = hitDistanceMetersValue;

    if(const auto selectedName = selectPrimaryFeatureName(feature); selectedName.has_value()) {
      entry.primaryName = selectedName->value;
      entry.nameSourceAttribute = selectedName->sourceAttribute;
    }

    const auto symbolization = symbolizer.symbolize(feature);
    entry.activeStyleKey = symbolization.styleKey;
    entry.textStyleKey = symbolization.textKey;
    entry.suppressed = symbolization.suppressed;
    if(symbolization.s52Lookup.has_value()) {
      entry.activeRuleId = symbolization.s52Lookup->ruleId;
      entry.viewGroup = symbolization.s52Lookup->viewGroup;
      entry.displayCategory = toPublicDisplayCategory(symbolization.s52Lookup->displayCategory);

      const auto descriptor = std::find_if(
        ruleDescriptors.begin(),
        ruleDescriptors.end(),
        [&](const S52RuleDescriptorEntry &candidate) {
          return candidate.ruleId == symbolization.s52Lookup->ruleId;
        });
      if(descriptor != ruleDescriptors.end()) {
        entry.activeRuleLabel = descriptor->label;
      }
    }
  };

  auto exportEntry = [](const FeatureSummaryEntry &entry, chart_view_feature_summary_t &dto) {
    dto = {};
    dto.runtime_feature_token = entry.runtimeFeatureToken;
    dto.feature_id = entry.featureId;
    dto.source_type = entry.sourceType;
    dto.dataset_name = entry.datasetName.empty() ? nullptr : entry.datasetName.c_str();
    dto.class_code = entry.classCode;
    dto.object_acronym = entry.objectAcronym.empty() ? nullptr : entry.objectAcronym.c_str();
    dto.geometry_type = entry.geometryType;
    dto.min_lon = entry.extent.minLon;
    dto.min_lat = entry.extent.minLat;
    dto.max_lon = entry.extent.maxLon;
    dto.max_lat = entry.extent.maxLat;
    dto.hit_distance_m = entry.hitDistanceMeters;
    dto.primary_name = entry.primaryName.empty() ? nullptr : entry.primaryName.c_str();
    dto.name_source_attribute =
      entry.nameSourceAttribute.empty() ? nullptr : entry.nameSourceAttribute.c_str();
    dto.active_rule_id = entry.activeRuleId.empty() ? nullptr : entry.activeRuleId.c_str();
    dto.active_rule_label = entry.activeRuleLabel.empty() ? nullptr : entry.activeRuleLabel.c_str();
    dto.active_style_key = entry.activeStyleKey.empty() ? nullptr : entry.activeStyleKey.c_str();
    dto.text_style_key = entry.textStyleKey.empty() ? nullptr : entry.textStyleKey.c_str();
    dto.view_group = entry.viewGroup;
    dto.display_category = entry.displayCategory;
    dto.suppressed = entry.suppressed ? 1U : 0U;
  };

  std::vector<FeatureSummaryEntry> matches;
  matches.reserve(16);

  std::uint32_t runtimeFeatureToken = 1U;
  auto collectMatches = [&](const chart_data::FeatureChartDataset &dataset) {
    for(const auto &feature : dataset.features()) {
      const auto hitDistance = hitDistanceMeters(feature, hitPoint, toleranceMeters);
      if(!hitDistance.has_value()) {
        ++runtimeFeatureToken;
        continue;
      }

      FeatureSummaryEntry entry;
      describeEntry(entry, dataset, feature, runtimeFeatureToken, *hitDistance);
      matches.push_back(std::move(entry));
      ++runtimeFeatureToken;
    }
  };

  if(m_dataset != nullptr) {
    collectMatches(*m_dataset);
  } else {
    for(const auto &dataset : m_quiltDatasets) {
      collectMatches(dataset);
    }
  }

  std::stable_sort(
    matches.begin(),
    matches.end(),
    [](const FeatureSummaryEntry &lhs, const FeatureSummaryEntry &rhs) {
      return std::tie(lhs.hitDistanceMeters, lhs.runtimeFeatureToken)
           < std::tie(rhs.hitDistanceMeters, rhs.runtimeFeatureToken);
    });

  const auto limitedCount = query.max_results == 0U
    ? matches.size()
    : (std::min)(matches.size(), static_cast<std::size_t>(query.max_results));
  m_featureQueryCache.assign(matches.begin(), matches.begin() + static_cast<std::ptrdiff_t>(limitedCount));

  const auto required = static_cast<std::uint32_t>(m_featureQueryCache.size());
  if(out == nullptr) {
    inoutCount = required;
    return chart_view_status_ok;
  }

  if(inoutCount < required) {
    inoutCount = required;
    return chart_view_status_invalid_argument;
  }

  for(std::uint32_t index = 0; index < required; ++index) {
    exportEntry(m_featureQueryCache[index], out[index]);
  }
  inoutCount = required;
  return chart_view_status_ok;
}

const RuntimeContext::FeatureSummaryEntry *RuntimeContext::findFeatureSummaryEntry(
  std::uint32_t runtimeFeatureToken) const
{
  if(runtimeFeatureToken == 0U) {
    return nullptr;
  }

  const auto cached = std::find_if(
    m_featureQueryCache.begin(),
    m_featureQueryCache.end(),
    [&](const FeatureSummaryEntry &entry) { return entry.runtimeFeatureToken == runtimeFeatureToken; });
  if(cached != m_featureQueryCache.end()) {
    return &(*cached);
  }

  auto symbolizerSettings = m_s52Settings;
  symbolizerSettings.viewingScaleDenominator = m_viewport.viewport().scale_denominator;
  portrayal::FeatureSymbolizer symbolizer(symbolizerSettings);
  if(!m_s57ClassFilters.empty()) {
    std::vector<portrayal::S57ClassSelectionFilter> classFilters;
    classFilters.reserve(m_s57ClassFilters.size());
    for(const auto &entry : m_s57ClassFilters) {
      classFilters.push_back({entry.objectAcronym, entry.enabled});
    }
    symbolizer.setS57ClassFilters(classFilters);
  }
  if(!m_s52RuleFilters.empty()) {
    std::vector<portrayal::S52RuleSelectionFilter> ruleFilters;
    ruleFilters.reserve(m_s52RuleFilters.size());
    for(const auto &entry : m_s52RuleFilters) {
      ruleFilters.push_back({entry.ruleId, entry.enabled});
    }
    symbolizer.setS52RuleFilters(ruleFilters);
  }
  const auto &ruleDescriptors = compiledRuleDescriptors();

  auto describeEntry = [&](FeatureSummaryEntry &entry,
                           const chart_data::FeatureChartDataset &dataset,
                           const chart_data::Feature &feature,
                           std::uint32_t token) {
    entry = {};
    entry.runtimeFeatureToken = token;
    entry.featureId = feature.id;
    entry.sourceType = dataset.meta().sourceType;
    entry.datasetName = dataset.meta().name;
    entry.classCode = feature.classCode;
    entry.objectAcronym = feature.classAcronym;
    switch(chart_data::geometryType(feature.geometry)) {
    case chart_data::GeometryType::kLine:
      entry.geometryType = chart_view_feature_geometry_line;
      break;
    case chart_data::GeometryType::kArea:
      entry.geometryType = chart_view_feature_geometry_area;
      break;
    case chart_data::GeometryType::kPoint:
    default:
      entry.geometryType = chart_view_feature_geometry_point;
      break;
    }
    entry.extent = computeFeatureExtent(feature);

    if(const auto selectedName = selectPrimaryFeatureName(feature); selectedName.has_value()) {
      entry.primaryName = selectedName->value;
      entry.nameSourceAttribute = selectedName->sourceAttribute;
    }

    const auto symbolization = symbolizer.symbolize(feature);
    entry.activeStyleKey = symbolization.styleKey;
    entry.textStyleKey = symbolization.textKey;
    entry.suppressed = symbolization.suppressed;
    if(symbolization.s52Lookup.has_value()) {
      entry.activeRuleId = symbolization.s52Lookup->ruleId;
      entry.viewGroup = symbolization.s52Lookup->viewGroup;
      entry.displayCategory = toPublicDisplayCategory(symbolization.s52Lookup->displayCategory);

      const auto descriptor = std::find_if(
        ruleDescriptors.begin(),
        ruleDescriptors.end(),
        [&](const S52RuleDescriptorEntry &candidate) {
          return candidate.ruleId == symbolization.s52Lookup->ruleId;
        });
      if(descriptor != ruleDescriptors.end()) {
        entry.activeRuleLabel = descriptor->label;
      }
    }
  };

  std::uint32_t token = 1U;
  auto findInDataset = [&](const chart_data::FeatureChartDataset &dataset) -> bool {
    for(const auto &feature : dataset.features()) {
      if(token == runtimeFeatureToken) {
        describeEntry(m_featureDescribeCache, dataset, feature, token);
        return true;
      }
      ++token;
    }
    return false;
  };

  if(m_dataset != nullptr) {
    if(findInDataset(*m_dataset)) {
      return &m_featureDescribeCache;
    }
  } else {
    for(const auto &dataset : m_quiltDatasets) {
      if(findInDataset(dataset)) {
        return &m_featureDescribeCache;
      }
    }
  }

  return nullptr;
}

chart_view_status_t RuntimeContext::describeFeature(
  std::uint32_t runtimeFeatureToken,
  chart_view_feature_summary_t &out) const
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  const auto *entry = findFeatureSummaryEntry(runtimeFeatureToken);
  if(entry == nullptr) {
    return chart_view_status_invalid_argument;
  }

  out = {};
  out.runtime_feature_token = entry->runtimeFeatureToken;
  out.feature_id = entry->featureId;
  out.source_type = entry->sourceType;
  out.dataset_name = entry->datasetName.empty() ? nullptr : entry->datasetName.c_str();
  out.class_code = entry->classCode;
  out.object_acronym = entry->objectAcronym.empty() ? nullptr : entry->objectAcronym.c_str();
  out.geometry_type = entry->geometryType;
  out.min_lon = entry->extent.minLon;
  out.min_lat = entry->extent.minLat;
  out.max_lon = entry->extent.maxLon;
  out.max_lat = entry->extent.maxLat;
  out.hit_distance_m = entry->hitDistanceMeters;
  out.primary_name = entry->primaryName.empty() ? nullptr : entry->primaryName.c_str();
  out.name_source_attribute =
    entry->nameSourceAttribute.empty() ? nullptr : entry->nameSourceAttribute.c_str();
  out.active_rule_id = entry->activeRuleId.empty() ? nullptr : entry->activeRuleId.c_str();
  out.active_rule_label = entry->activeRuleLabel.empty() ? nullptr : entry->activeRuleLabel.c_str();
  out.active_style_key = entry->activeStyleKey.empty() ? nullptr : entry->activeStyleKey.c_str();
  out.text_style_key = entry->textStyleKey.empty() ? nullptr : entry->textStyleKey.c_str();
  out.view_group = entry->viewGroup;
  out.display_category = entry->displayCategory;
  out.suppressed = entry->suppressed ? 1U : 0U;
  return chart_view_status_ok;
}

void RuntimeContext::getLoadedChartInfo(chart_view_loaded_chart_info_t &out) const
{
  out = {};

  if(m_dataset) {
    const auto &meta = m_dataset->meta();
    out.source_type = meta.sourceType;
    out.feature_count = static_cast<std::uint32_t>(m_dataset->featureCount());
    out.min_lon = meta.extent.minLon;
    out.min_lat = meta.extent.minLat;
    out.max_lon = meta.extent.maxLon;
    out.max_lat = meta.extent.maxLat;
    return;
  }

  if(!m_directoryMode) {
    out.source_type = chart_view_chart_source_unknown;
    return;
  }

  Extent extent = invalidExtent();
  std::uint64_t featureCount = 0;
  chart_view_chart_source_type_t sourceType = chart_view_chart_source_unknown;
  bool mixedTypes = false;

  if(!m_quiltDatasets.empty()) {
    for(const auto &dataset : m_quiltDatasets) {
      const auto &meta = dataset.meta();
      extent = unionExtents(extent, meta.extent);
      featureCount += dataset.featureCount();

      if(sourceType == chart_view_chart_source_unknown) {
        sourceType = meta.sourceType;
      } else if(meta.sourceType != sourceType) {
        mixedTypes = true;
      }
    }
  } else {
    for(const auto &entry : m_catalog.entries()) {
      extent = unionExtents(extent, entry.extent);
      if(sourceType == chart_view_chart_source_unknown) {
        sourceType = entry.sourceType;
      } else if(entry.sourceType != sourceType) {
        mixedTypes = true;
      }
    }
  }

  out.source_type = mixedTypes ? chart_view_chart_source_unknown : sourceType;
  out.feature_count = static_cast<std::uint32_t>(
    std::min<std::uint64_t>(featureCount, std::numeric_limits<std::uint32_t>::max()));
  if(extent.isValid()) {
    out.min_lon = extent.minLon;
    out.min_lat = extent.minLat;
    out.max_lon = extent.maxLon;
    out.max_lat = extent.maxLat;
  }
}

void RuntimeContext::getViewport(chart_view_viewport_t &out) const
{
  out = m_viewport.viewport();
}

chart_view_status_t RuntimeContext::renderFrame(chart_view_render_frame_result_t &out)
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  out = {};

  if(!m_viewport.isValid()) {
    return chart_view_status_ok;
  }

  const auto targetStatus = ensureRenderTarget();
  if(targetStatus != chart_view_status_ok) {
    return targetStatus;
  }

  if(m_directoryMode) {
    if(m_quiltPlan.empty() || m_quiltDatasets.empty()) {
      (void)m_renderBackend.renderClearFrame(kFrameClearR, kFrameClearG, kFrameClearB, kFrameClearA);
      return chart_view_status_ok;
    }

    SceneBuilderFromSenc builder;
    auto snapshot = builder.build(
      m_quiltPlan,
      std::span<const FeatureChartDataset>(m_quiltDatasets),
      m_viewport);

    if(!snapshot || snapshot->empty()) {
      (void)m_renderBackend.renderClearFrame(kFrameClearR, kFrameClearG, kFrameClearB, kFrameClearA);
      return chart_view_status_ok;
    }

    m_scheduler.submitSnapshot(snapshot);

    const auto renderResult =
      m_renderer.render(*snapshot, std::span<const FeatureChartDataset>(m_quiltDatasets), m_renderBackend);
    if(renderResult.status != chart_view_status_ok) {
      return renderResult.status;
    }

    out.points_rendered = renderResult.pointsRendered;
    out.lines_rendered = renderResult.linesRendered;
    out.areas_rendered = renderResult.areasRendered;
    out.total_vertices = renderResult.totalVertices;
    return chart_view_status_ok;
  }

  if(!m_dataset) {
    return chart_view_status_ok;
  }

  SceneBuilderFromSenc builder;
  auto snapshot = builder.build(*m_dataset, m_viewport);
  if(!snapshot || snapshot->empty()) {
    (void)m_renderBackend.renderClearFrame(kFrameClearR, kFrameClearG, kFrameClearB, kFrameClearA);
    return chart_view_status_ok;
  }

  m_scheduler.submitSnapshot(snapshot);

  const auto renderResult = m_renderer.render(*snapshot, *m_dataset, m_renderBackend);
  if(renderResult.status != chart_view_status_ok) {
    return renderResult.status;
  }

  out.points_rendered = renderResult.pointsRendered;
  out.lines_rendered = renderResult.linesRendered;
  out.areas_rendered = renderResult.areasRendered;
  out.total_vertices = renderResult.totalVertices;
  return chart_view_status_ok;
}

void RuntimeContext::getFrameBufferInfo(chart_view_frame_buffer_info_t &out) const
{
  out = {};
  if(!m_renderBackend.isInitialized()) {
    return;
  }

  out.pixel_width = static_cast<std::uint32_t>(m_renderBackend.surfaceWidth());
  out.pixel_height = static_cast<std::uint32_t>(m_renderBackend.surfaceHeight());
  out.stride_bytes = m_renderBackend.strideBytes();
  out.rgba_size_bytes = static_cast<std::uint32_t>(m_renderBackend.frameByteSize());
}

chart_view_status_t RuntimeContext::copyFrameRgba(std::span<std::uint8_t> dst) const
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  return m_renderBackend.copyFrameRgba(dst);
}

}// namespace chart_view::runtime
