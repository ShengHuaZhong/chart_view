#include <catch2/catch_test_macros.hpp>

#include "catalog/chart_catalog.hpp"
#include "catalog/chart_selection_policy.hpp"
#include "catalog/coverage_index.hpp"
#include "feature_layer_renderer.hpp"
#include "quilt/quilt_planner.hpp"
#include "quilt/zoom_policy.hpp"
#include "rhi_render_backend.hpp"
#include "s101/s101_reader.hpp"
#include "scene_builder_from_senc.hpp"
#include "senc/senc_reader.hpp"
#include "senc/senc_writer.hpp"

#include <QByteArray>
#include <QGuiApplication>
#include <QtGlobal>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
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
char *AppGuard::argv[] = {const_cast<char *>("s101_quilt_smoke_tests"), nullptr};

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
  meta.name = sourcePath.stem().string();
  meta.sourceType = chart_view_chart_source_s101;

  if(meta.nativeScale <= 0.0) {
    meta.nativeScale = estimateScaleForExtent(meta.extent, 1280, 720);
  }

  if(meta.usageBand == 0U) {
    meta.usageBand = usageBandForScale(meta.nativeScale);
  }

  dataset.setMeta(std::move(meta));
  return dataset;
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

void writeText(const std::filesystem::path &path, std::string_view text)
{
  std::ofstream stream(path, std::ios::binary);
  REQUIRE(stream.is_open());
  stream.write(text.data(), static_cast<std::streamsize>(text.size()));
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

std::array<LoadedChart, 2> loadSyntheticCharts(const std::filesystem::path &sourceDirectory)
{
  static constexpr std::string_view kChartA = R"(S101SMOKE
name=s101_quilt_a
native_scale=12000
point=121.8006,31.2301
line=121.8002,31.2298;121.8013,31.2304;121.8020,31.2299
area=121.8004,31.2300;121.8014,31.2300;121.8014,31.2308;121.8004,31.2308
)";

  static constexpr std::string_view kChartB = R"(S101SMOKE
name=s101_quilt_b
native_scale=15000
point=121.8016,31.2302
line=121.8010,31.2299;121.8022,31.2305;121.8031,31.2301
area=121.8012,31.2301;121.8025,31.2301;121.8025,31.2310;121.8012,31.2310
)";

  const auto sourcePathA = sourceDirectory / "s101_quilt_a.101";
  const auto sourcePathB = sourceDirectory / "s101_quilt_b.101";
  writeText(sourcePathA, kChartA);
  writeText(sourcePathB, kChartB);

  chart_view::runtime::s101::S101Reader reader;
  const auto resultA = reader.read(sourcePathA.string());
  const auto resultB = reader.read(sourcePathB.string());
  REQUIRE(resultA.ok);
  REQUIRE(resultB.ok);
  REQUIRE_FALSE(resultA.dataset.empty());
  REQUIRE_FALSE(resultB.dataset.empty());
  REQUIRE(resultA.dataset.meta().extent.isValid());
  REQUIRE(resultB.dataset.meta().extent.isValid());

  return {
    LoadedChart{sourcePathA, prepareDatasetForSmoke(sourcePathA, resultA.dataset)},
    LoadedChart{sourcePathB, prepareDatasetForSmoke(sourcePathB, resultB.dataset)}};
}

}// namespace

TEST_CASE("S101 quilt smoke renders and zooms two synthetic charts", "[s101][quilt][smoke][rhi]")
{
  AppGuard guard;

  const auto tempPath = std::filesystem::temp_directory_path() / "chart_view_s101_quilt_smoke";
  {
    std::error_code ec;
    std::filesystem::remove_all(tempPath, ec);
  }
  REQUIRE(std::filesystem::create_directories(tempPath));
  TempDirGuard tempDir(tempPath);

  const auto sourceDirectory = tempDir.path / "sources";
  const auto catalogDirectory = tempDir.path / "catalog";
  REQUIRE(std::filesystem::create_directories(sourceDirectory));
  REQUIRE(std::filesystem::create_directories(catalogDirectory));

  const auto charts = loadSyntheticCharts(sourceDirectory);
  INFO("chart A: " << charts[0].sourcePath.string());
  INFO("chart B: " << charts[1].sourcePath.string());

  chart_view::runtime::senc::SencWriter writer;
  for(const auto &chart : charts) {
    INFO("catalog chart: " << chart.dataset.meta().name);
    INFO("native scale: " << chart.dataset.meta().nativeScale);
    INFO("feature count: " << chart.dataset.featureCount());

    const auto blob = writer.write(chart.dataset);
    REQUIRE_FALSE(blob.empty());
    writeBlob(catalogDirectory / (chart.dataset.meta().name + ".senc"), blob);
  }

  chart_view::runtime::catalog::ChartCatalog catalog;
  REQUIRE(catalog.loadDirectory(catalogDirectory));
  REQUIRE(catalog.size() == 2);

  chart_view::runtime::catalog::CoverageIndex coverageIndex;
  REQUIRE(coverageIndex.build(catalog));

  const auto combinedExtent =
    unionExtents(charts[0].dataset.meta().extent, charts[1].dataset.meta().extent);
  constexpr int kViewportWidth = 1600;
  constexpr int kViewportHeight = 900;
  const auto fittedScale = estimateScaleForExtent(combinedExtent, kViewportWidth, kViewportHeight);
  const auto baseScale = std::max(fittedScale, 20000.0);
  const auto baseViewport = makeViewport(combinedExtent, baseScale, kViewportWidth, kViewportHeight);

  chart_view::runtime::catalog::ChartSelectionPolicy selectionPolicy;
  chart_view::runtime::quilt::QuiltPlanner planner;
  const auto basePlan = planner.build(
    coverageIndex,
    selectionPolicy,
    computeViewportExtent(baseViewport),
    baseViewport.scale_denominator);

  REQUIRE(basePlan.layers().size() == 2);
  REQUIRE(basePlan.layers()[0].sourceType == chart_view_chart_source_s101);
  REQUIRE(basePlan.layers()[1].sourceType == chart_view_chart_source_s101);

  const auto baseDatasets = loadPlanDatasets(basePlan);
  REQUIRE(baseDatasets.size() == 2);

  ViewportState viewportState;
  viewportState.set(baseViewport);

  chart_view::runtime::SceneBuilderFromSenc builder;
  const auto baseSnapshot = builder.build(
    basePlan,
    std::span<const FeatureChartDataset>(baseDatasets),
    viewportState);
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
