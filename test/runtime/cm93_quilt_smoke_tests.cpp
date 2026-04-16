#include <catch2/catch_test_macros.hpp>

#include "catalog/chart_catalog.hpp"
#include "catalog/chart_selection_policy.hpp"
#include "catalog/coverage_index.hpp"
#include "cm93/cm93_reader.hpp"
#include "feature_layer_renderer.hpp"
#include "quilt/quilt_planner.hpp"
#include "quilt/zoom_policy.hpp"
#include "rhi_render_backend.hpp"
#include "scene_builder_from_senc.hpp"
#include "senc/senc_reader.hpp"
#include "senc/senc_writer.hpp"

#include <QByteArray>
#include <QGuiApplication>
#include <QtGlobal>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace {
using chart_view::runtime::ViewportState;
using chart_view::runtime::chart_data::Extent;
using chart_view::runtime::chart_data::FeatureChartDataset;

struct AppGuard
{
  static int argc;
  static char *argv[];

  AppGuard()
  {
    if(qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
      qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    }
  }

  QGuiApplication app{argc, argv};
};

int AppGuard::argc = 1;
char *AppGuard::argv[] = {const_cast<char *>("cm93_quilt_smoke_tests"), nullptr};

struct TempDirGuard
{
  explicit TempDirGuard(std::filesystem::path directory)
    : path(std::move(directory))
  {
  }

  ~TempDirGuard()
  {
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
  }

  std::filesystem::path path;
};

struct LoadedChart
{
  std::filesystem::path sourcePath;
  FeatureChartDataset dataset;
};

std::filesystem::path getCm93Root()
{
#ifdef CHARTSYS_CM93_TESTDATA_ROOT
  const char *root = CHARTSYS_CM93_TESTDATA_ROOT;
#else
  const char *root = std::getenv("CHARTSYS_CM93_TESTDATA_ROOT");
#endif

  if(root == nullptr || !std::filesystem::exists(root)) {
    SKIP("CHARTSYS_CM93_TESTDATA_ROOT not set or missing");
  }

  return root;
}

bool isCm93ChartDirectoryName(std::string_view name)
{
  if(name.size() != 8) {
    return false;
  }

  return std::all_of(
    name.begin(),
    name.end(),
    [](unsigned char ch) { return std::isdigit(ch) != 0; });
}

bool isCm93DetailDirectoryName(std::string_view name)
{
  return name.size() == 1 && std::isalpha(static_cast<unsigned char>(name.front())) != 0;
}

bool isCm93CellFilePath(const std::filesystem::path &path)
{
  const auto extension = path.extension().string();
  if(extension.size() != 2 || extension.front() != '.') {
    return false;
  }

  if(std::isalpha(static_cast<unsigned char>(extension.back())) == 0) {
    return false;
  }

  return isCm93ChartDirectoryName(path.stem().string());
}

std::vector<std::filesystem::path> findCm93ChartDirectories(const std::filesystem::path &root)
{
  std::vector<std::filesystem::path> chartDirectories;

  for(const auto &entry : std::filesystem::directory_iterator(root)) {
    if(!entry.is_directory()) {
      continue;
    }

    const auto directoryName = entry.path().filename().string();
    if(isCm93ChartDirectoryName(directoryName)) {
      chartDirectories.push_back(entry.path());
    }
  }

  std::sort(chartDirectories.begin(), chartDirectories.end());
  return chartDirectories;
}

std::vector<std::filesystem::path> findCm93CellFilesInChartDirectory(
  const std::filesystem::path &chartDirectory)
{
  std::vector<std::filesystem::path> cellFiles;

  for(const auto &entry : std::filesystem::directory_iterator(chartDirectory)) {
    if(entry.is_directory()) {
      const auto directoryName = entry.path().filename().string();
      if(!isCm93DetailDirectoryName(directoryName)) {
        continue;
      }

      for(const auto &cellFile : std::filesystem::directory_iterator(entry.path())) {
        if(cellFile.is_regular_file() && isCm93CellFilePath(cellFile.path())) {
          cellFiles.push_back(cellFile.path());
        }
      }

      continue;
    }

    if(entry.is_regular_file() && isCm93CellFilePath(entry.path())) {
      cellFiles.push_back(entry.path());
    }
  }

  std::sort(cellFiles.begin(), cellFiles.end());
  return cellFiles;
}

double metresPerDegreeLon(double centerLat) noexcept
{
  constexpr double kMetresPerDegLat = 111320.0;
  const auto cosLat = std::cos(centerLat * 3.14159265358979323846 / 180.0);
  return kMetresPerDegLat * (cosLat > 1e-6 ? cosLat : 1e-6);
}

double estimateScaleForExtent(const Extent &extent, int pixelWidth, int pixelHeight) noexcept
{
  constexpr double kMetresPerDegLat = 111320.0;
  constexpr double kPixelsPerMetre = 3779.5275591;

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

std::uint32_t usageBandForScale(double scaleDenominator) noexcept
{
  if(scaleDenominator <= 22000.0) {
    return 6;
  }
  if(scaleDenominator <= 90000.0) {
    return 5;
  }
  if(scaleDenominator <= 350000.0) {
    return 4;
  }
  if(scaleDenominator <= 1500000.0) {
    return 3;
  }
  if(scaleDenominator <= 4000000.0) {
    return 2;
  }
  return 1;
}

FeatureChartDataset prepareDatasetForSmoke(std::filesystem::path sourcePath, FeatureChartDataset dataset)
{
  auto meta = dataset.meta();
  meta.name = sourcePath.filename().string();
  meta.sourceType = chart_view_chart_source_cm93;

  if(meta.nativeScale <= 0.0) {
    meta.nativeScale = estimateScaleForExtent(meta.extent, 1280, 720);
  }

  if(meta.usageBand == 0U) {
    meta.usageBand = usageBandForScale(meta.nativeScale);
  }

  dataset.setMeta(std::move(meta));
  return dataset;
}

bool extentsOverlapOrTouch(const Extent &lhs, const Extent &rhs) noexcept
{
  return lhs.minLon <= rhs.maxLon && lhs.maxLon >= rhs.minLon && lhs.minLat <= rhs.maxLat
      && lhs.maxLat >= rhs.minLat;
}

Extent unionExtents(const Extent &lhs, const Extent &rhs) noexcept
{
  return {
    std::min(lhs.minLon, rhs.minLon),
    std::min(lhs.minLat, rhs.minLat),
    std::max(lhs.maxLon, rhs.maxLon),
    std::max(lhs.maxLat, rhs.maxLat)};
}

chart_view_viewport_t makeViewport(const Extent &extent, double scaleDenominator, int width, int height) noexcept
{
  chart_view_viewport_t viewport{};
  viewport.center_lon = (extent.minLon + extent.maxLon) * 0.5;
  viewport.center_lat = (extent.minLat + extent.maxLat) * 0.5;
  viewport.scale_denominator = scaleDenominator;
  viewport.pixel_width = width;
  viewport.pixel_height = height;
  return viewport;
}

Extent computeViewportExtent(const chart_view_viewport_t &viewport) noexcept
{
  constexpr double kMetresPerDegLat = 111320.0;
  constexpr double kPixelsPerMetre = 3779.5275591;

  const auto width = std::max(viewport.pixel_width, 1);
  const auto height = std::max(viewport.pixel_height, 1);
  const auto halfWidthDeg =
    (static_cast<double>(width) * 0.5) / kPixelsPerMetre * viewport.scale_denominator
    / metresPerDegreeLon(viewport.center_lat);
  const auto halfHeightDeg =
    (static_cast<double>(height) * 0.5) / kPixelsPerMetre * viewport.scale_denominator
    / kMetresPerDegLat;

  return {
    viewport.center_lon - halfWidthDeg,
    viewport.center_lat - halfHeightDeg,
    viewport.center_lon + halfWidthDeg,
    viewport.center_lat + halfHeightDeg};
}

void writeBlob(const std::filesystem::path &path, std::span<const std::uint8_t> blob)
{
  std::ofstream stream(path, std::ios::binary);
  REQUIRE(stream.is_open());
  stream.write(reinterpret_cast<const char *>(blob.data()), static_cast<std::streamsize>(blob.size()));
  REQUIRE(stream.good());
}

std::vector<FeatureChartDataset> loadPlanDatasets(const chart_view::runtime::quilt::QuiltPlan &plan)
{
  chart_view::runtime::senc::SencReader reader;
  std::vector<FeatureChartDataset> datasets;
  datasets.reserve(plan.layers().size());

  for(const auto &layer : plan.layers()) {
    const auto result = reader.readFromFile(layer.sencPath.string());
    REQUIRE(result.ok);
    datasets.push_back(result.dataset);
  }

  return datasets;
}

std::optional<LoadedChart> loadBestChartDirectory(
  chart_view::runtime::cm93::Cm93Reader &reader,
  const std::filesystem::path &chartDirectory)
{
  const auto cellFiles = findCm93CellFilesInChartDirectory(chartDirectory);

  std::optional<LoadedChart> bestChart;
  std::size_t bestFeatureCount = 0;

  for(const auto &cellFile : cellFiles) {
    const auto result = reader.read(cellFile.string());
    if(!result.ok || result.dataset.empty() || !result.dataset.meta().extent.isValid()) {
      continue;
    }

    const auto featureCount = result.dataset.featureCount();
    if(!bestChart.has_value() || featureCount > bestFeatureCount) {
      bestFeatureCount = featureCount;
      bestChart = LoadedChart{
        chartDirectory,
        prepareDatasetForSmoke(chartDirectory, result.dataset)};
    }
  }

  return bestChart;
}

std::optional<std::array<LoadedChart, 2>> selectBestPair(const std::filesystem::path &root)
{
  const auto chartDirectories = findCm93ChartDirectories(root);
  if(chartDirectories.size() < 2) {
    return std::nullopt;
  }

  chart_view::runtime::cm93::Cm93Reader reader;
  std::vector<LoadedChart> loadedCharts;
  loadedCharts.reserve(chartDirectories.size());

  for(const auto &chartDirectory : chartDirectories) {
    const auto loadedChart = loadBestChartDirectory(reader, chartDirectory);
    if(loadedChart.has_value()) {
      loadedCharts.push_back(std::move(*loadedChart));
    }
  }

  if(loadedCharts.size() < 2) {
    return std::nullopt;
  }

  std::size_t bestLeft = 0;
  std::size_t bestRight = 0;
  auto bestUnionArea = std::numeric_limits<double>::max();
  bool foundPair = false;

  for(std::size_t left = 0; left < loadedCharts.size(); ++left) {
    for(std::size_t right = left + 1; right < loadedCharts.size(); ++right) {
      const auto &leftExtent = loadedCharts[left].dataset.meta().extent;
      const auto &rightExtent = loadedCharts[right].dataset.meta().extent;
      if(!extentsOverlapOrTouch(leftExtent, rightExtent)) {
        continue;
      }

      const auto combinedExtent = unionExtents(leftExtent, rightExtent);
      const auto unionArea =
        std::max(0.0, combinedExtent.maxLon - combinedExtent.minLon)
        * std::max(0.0, combinedExtent.maxLat - combinedExtent.minLat);
      if(unionArea < bestUnionArea) {
        bestUnionArea = unionArea;
        bestLeft = left;
        bestRight = right;
        foundPair = true;
      }
    }
  }

  if(!foundPair) {
    return std::nullopt;
  }

  return std::array<LoadedChart, 2>{loadedCharts[bestLeft], loadedCharts[bestRight]};
}
}// namespace

TEST_CASE("CM93 quilt smoke renders and zooms two real chart directories", "[cm93][quilt][smoke][real-data][rhi]")
{
  AppGuard guard;

  const auto root = getCm93Root();
  const auto charts = selectBestPair(root);
  if(!charts.has_value()) {
    INFO("Current CM93 data/decoder did not yield two readable non-empty adjacent chart directories");
    CHECK(true);
    return;
  }

  INFO("chart directory A: " << (*charts)[0].sourcePath.string());
  INFO("chart directory B: " << (*charts)[1].sourcePath.string());

  const auto tempPath = std::filesystem::temp_directory_path() / "chart_view_cm93_quilt_smoke";
  {
    std::error_code ec;
    std::filesystem::remove_all(tempPath, ec);
  }
  REQUIRE(std::filesystem::create_directories(tempPath));
  TempDirGuard tempDir(tempPath);

  chart_view::runtime::senc::SencWriter writer;
  for(const auto &chart : *charts) {
    INFO("catalog chart: " << chart.dataset.meta().name);
    INFO("native scale: " << chart.dataset.meta().nativeScale);
    INFO("feature count: " << chart.dataset.featureCount());

    const auto blob = writer.write(chart.dataset);
    REQUIRE_FALSE(blob.empty());
    writeBlob(tempDir.path / (chart.dataset.meta().name + ".senc"), blob);
  }

  chart_view::runtime::catalog::ChartCatalog catalog;
  REQUIRE(catalog.loadDirectory(tempDir.path));
  REQUIRE(catalog.size() == 2);

  chart_view::runtime::catalog::CoverageIndex coverageIndex;
  REQUIRE(coverageIndex.build(catalog));

  const auto combinedExtent =
    unionExtents((*charts)[0].dataset.meta().extent, (*charts)[1].dataset.meta().extent);
  constexpr int kViewportWidth = 1600;
  constexpr int kViewportHeight = 900;
  const auto baseScale = estimateScaleForExtent(combinedExtent, kViewportWidth, kViewportHeight);
  const auto baseViewport = makeViewport(combinedExtent, baseScale, kViewportWidth, kViewportHeight);

  chart_view::runtime::catalog::ChartSelectionPolicy selectionPolicy;
  chart_view::runtime::quilt::QuiltPlanner planner;
  const auto basePlan = planner.build(
    coverageIndex,
    selectionPolicy,
    computeViewportExtent(baseViewport),
    baseViewport.scale_denominator);

  REQUIRE(basePlan.layers().size() == 2);
  REQUIRE(basePlan.layers()[0].sourceType == chart_view_chart_source_cm93);
  REQUIRE(basePlan.layers()[1].sourceType == chart_view_chart_source_cm93);

  const auto baseDatasets = loadPlanDatasets(basePlan);
  REQUIRE(baseDatasets.size() == 2);

  ViewportState viewportState;
  viewportState.set(baseViewport);

  chart_view::runtime::SceneBuilderFromSenc builder;
  const auto baseSnapshot = builder.build(basePlan, std::span<const FeatureChartDataset>(baseDatasets), viewportState);
  REQUIRE(baseSnapshot != nullptr);
  REQUIRE(baseSnapshot->charts().size() == 2);
  REQUIRE_FALSE(baseSnapshot->empty());

  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(kViewportWidth, kViewportHeight) == chart_view_status_ok);

  chart_view::runtime::FeatureLayerRenderer renderer;
  const auto baseRender = renderer.render(
    *baseSnapshot,
    std::span<const FeatureChartDataset>(baseDatasets),
    backend);
  REQUIRE(baseRender.status == chart_view_status_ok);
  REQUIRE(baseRender.totalVertices > 0);

  chart_view::runtime::quilt::ZoomPolicy zoomPolicy;
  const auto requestedZoomScale = zoomPolicy.zoomIn(baseViewport.scale_denominator, 1);
  const auto zoomDecision = zoomPolicy.evaluate(basePlan, requestedZoomScale);
  REQUIRE(zoomDecision.resolvedScaleDenominator > 0.0);
  REQUIRE(zoomDecision.resolvedScaleDenominator < baseViewport.scale_denominator);

  auto zoomedViewport = baseViewport;
  zoomedViewport.scale_denominator = zoomDecision.resolvedScaleDenominator;

  ViewportState zoomedViewportState;
  zoomedViewportState.set(zoomedViewport);

  const auto zoomedPlan = planner.build(
    coverageIndex,
    selectionPolicy,
    computeViewportExtent(zoomedViewport),
    zoomedViewport.scale_denominator);
  REQUIRE_FALSE(zoomedPlan.empty());

  const auto zoomedDatasets = loadPlanDatasets(zoomedPlan);
  REQUIRE_FALSE(zoomedDatasets.empty());

  const auto zoomedSnapshot = builder.build(
    zoomedPlan,
    std::span<const FeatureChartDataset>(zoomedDatasets),
    zoomedViewportState);
  REQUIRE(zoomedSnapshot != nullptr);
  REQUIRE_FALSE(zoomedSnapshot->empty());

  const auto zoomedRender = renderer.render(
    *zoomedSnapshot,
    std::span<const FeatureChartDataset>(zoomedDatasets),
    backend);
  REQUIRE(zoomedRender.status == chart_view_status_ok);
  REQUIRE(zoomedRender.totalVertices > 0);
}
