#include "runtime_context.hpp"

#include "cm93/cm93_reader.hpp"
#include "quilt/quilt_planner.hpp"
#include "quilt/zoom_policy.hpp"
#include "s101/s101_reader.hpp"
#include "s57/s57_reader.hpp"
#include "scene_builder_from_senc.hpp"
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
#include <unordered_set>
#include <vector>

namespace {
using chart_view::runtime::ViewportState;
using chart_view::runtime::catalog::ChartCatalog;
using chart_view::runtime::catalog::ChartSelectionPolicy;
using chart_view::runtime::catalog::CoverageIndex;
using chart_view::runtime::chart_data::Extent;
using chart_view::runtime::chart_data::FeatureChartDataset;
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

  const auto halfWidthDeg =
    (static_cast<double>(vp.pixel_width) * 0.5) / kPixelsPerMetre * vp.scale_denominator
    / metresPerDegreeLon(vp.center_lat);
  const auto halfHeightDeg =
    (static_cast<double>(vp.pixel_height) * 0.5) / kPixelsPerMetre * vp.scale_denominator
    / kMetresPerDegLat;

  return {
    vp.center_lon - halfWidthDeg,
    vp.center_lat - halfHeightDeg,
    vp.center_lon + halfWidthDeg,
    vp.center_lat + halfHeightDeg};
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
