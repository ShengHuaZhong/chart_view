#include <catch2/catch_test_macros.hpp>

#include "feature_layer_renderer.hpp"
#include "label_layout.hpp"
#include "projection/projected_viewport.hpp"
#include "rhi_render_backend.hpp"
#include "scene_builder_from_senc.hpp"
#include "s57/s57_reader.hpp"
#include "senc/senc_reader.hpp"
#include "senc/senc_writer.hpp"
#include "text_label_renderer.hpp"
#include "portrayal/feature_symbolizer.hpp"
#include "portrayal/s52_display_settings.hpp"

#include <QByteArray>
#include <QGuiApplication>
#include <QtGlobal>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_set>
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
char *AppGuard::argv[] = {const_cast<char *>("s57_real_chart_smoke_tests"), nullptr};

struct LoadedChart
{
  std::filesystem::path sourcePath;
  chart_view::runtime::s57::S57ReadResult readResult;
  FeatureChartDataset preparedDataset;
};

struct VisibleLabelAuditStats
{
  std::size_t totalVisibleLabels{0};
  std::size_t visibleUnicodeLabels{0};
};

constexpr std::string_view kTargetChartAStem = "C1511781";
constexpr std::string_view kTargetChartBStem = "C1511782";

std::filesystem::path getS57Root()
{
#ifdef CHARTSYS_S57_TESTDATA_ROOT
  const char *root = CHARTSYS_S57_TESTDATA_ROOT;
#else
  const char *root = std::getenv("CHARTSYS_S57_TESTDATA_ROOT");
#endif

  if(root == nullptr || !std::filesystem::exists(root)) {
    SKIP("CHARTSYS_S57_TESTDATA_ROOT not set or missing");
  }

  return root;
}

std::vector<std::filesystem::path> findS57Charts(const std::filesystem::path &root)
{
  std::vector<std::filesystem::path> charts;
  for(const auto &entry : std::filesystem::recursive_directory_iterator(root)) {
    if(entry.is_regular_file() && entry.path().extension() == ".000") {
      charts.push_back(entry.path());
    }
  }

  std::sort(charts.begin(), charts.end());
  return charts;
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
  meta.name = sourcePath.stem().string();
  meta.sourceType = chart_view_chart_source_s57;

  if(meta.nativeScale <= 0.0) {
    meta.nativeScale = estimateScaleForExtent(meta.extent, 1280, 720);
  }

  if(meta.usageBand == 0U) {
    meta.usageBand = usageBandForScale(meta.nativeScale);
  }

  dataset.setMeta(std::move(meta));
  return dataset;
}

chart_view_viewport_t makeViewportForDataset(const FeatureChartDataset &dataset) noexcept
{
  chart_view_viewport_t viewport{};
  viewport.pixel_width = 1280;
  viewport.pixel_height = 720;
  viewport.center_lon = (dataset.meta().extent.minLon + dataset.meta().extent.maxLon) * 0.5;
  viewport.center_lat = (dataset.meta().extent.minLat + dataset.meta().extent.maxLat) * 0.5;
  viewport.scale_denominator = estimateScaleForExtent(dataset.meta().extent, viewport.pixel_width, viewport.pixel_height);
  return viewport;
}

std::size_t countS52SymbolizedFeatures(
  const FeatureChartDataset &dataset,
  const chart_view::runtime::portrayal::FeatureSymbolizer &symbolizer)
{
  std::size_t count = 0;
  for(const auto &feature : dataset.features()) {
    if(symbolizer.symbolize(feature).s52Lookup.has_value()) {
      ++count;
    }
  }
  return count;
}

std::size_t countNamedFeatures(const FeatureChartDataset &dataset)
{
  return static_cast<std::size_t>(std::count_if(
    dataset.features().begin(),
    dataset.features().end(),
    [](const auto &feature) {
      return chart_view::runtime::label::selectMultilingualLabelText(feature).has_value();
    }));
}

std::size_t countUnicodeNamedFeatures(const FeatureChartDataset &dataset)
{
  std::size_t count = 0;
  for(const auto &feature : dataset.features()) {
    const auto selected = chart_view::runtime::label::selectMultilingualLabelText(feature);
    if(!selected.has_value()) {
      continue;
    }

    if(std::any_of(
         selected->text.codePoints.begin(),
         selected->text.codePoints.end(),
         [](char32_t codePoint) { return codePoint > 0x7FU; })) {
      ++count;
    }
  }

  return count;
}

std::size_t countTextLabelCandidates(
  const FeatureChartDataset &dataset,
  const chart_view::runtime::portrayal::FeatureSymbolizer &symbolizer)
{
  std::size_t count = 0;
  for(const auto &feature : dataset.features()) {
    if(!symbolizer.symbolize(feature).textKey.empty()) {
      ++count;
    }
  }
  return count;
}

bool hasNonAsciiGlyph(const chart_view::runtime::LabelItem &label)
{
  return std::any_of(
    label.glyphText.begin(),
    label.glyphText.end(),
    [](char32_t codePoint) { return codePoint > 0x7FU; });
}

VisibleLabelAuditStats collectVisibleProjectedLabelStats(
  const chart_view::runtime::SceneSnapshot &snapshot,
  const FeatureChartDataset &dataset,
  const chart_view::runtime::portrayal::FeatureSymbolizer &symbolizer,
  const chart_view::runtime::portrayal::PortrayalRegistry &registry,
  const chart_view::runtime::projection::ProjectionContext &projectionContext,
  const chart_view::runtime::projection::ProjectedViewport &projectedViewport)
{
  chart_view::runtime::TextLabelRenderer labelRenderer;
  std::vector<chart_view::runtime::label::LabelBounds> occupiedBounds;
  VisibleLabelAuditStats stats;

  for(const auto &entry : snapshot.layers()) {
    const auto &features = dataset.features();
    if(entry.featureIndex >= features.size()) {
      continue;
    }

    const auto &feature = features[entry.featureIndex];
    const auto symbolization = symbolizer.symbolize(feature);
    if(symbolization.suppressed || symbolization.textKey.empty()) {
      continue;
    }

    chart_view::runtime::SurfacePoint anchor{};
    if(!chart_view::runtime::label::resolveProjectedLabelAnchor(
         feature,
         projectionContext,
         projectedViewport,
         anchor)) {
      continue;
    }

    const auto &rule = registry.resolveTextRuleForStyle(symbolization.textKey);
    auto label = labelRenderer.layout(
      symbolization.textKey,
      feature,
      anchor,
      rule,
      symbolization.textAttributeKey);
    if(!label.has_value()
       || !chart_view::runtime::label::labelBoundsVisible(
         label->bounds,
         projectedViewport.pixelWidth,
         projectedViewport.pixelHeight)) {
      continue;
    }

    const auto overlapsExisting = std::any_of(
      occupiedBounds.begin(),
      occupiedBounds.end(),
      [&](const chart_view::runtime::label::LabelBounds &existingBounds) {
        return chart_view::runtime::label::labelBoundsOverlap(existingBounds, label->bounds);
      });
    if(overlapsExisting) {
      continue;
    }

    occupiedBounds.push_back(label->bounds);
    ++stats.totalVisibleLabels;
    if(hasNonAsciiGlyph(*label)) {
      ++stats.visibleUnicodeLabels;
    }
  }

  return stats;
}

bool frameHasNonBackgroundPixel(std::span<const std::uint8_t> rgba)
{
  constexpr std::array<std::uint8_t, 4> background{230U, 230U, 217U, 255U};

  for(std::size_t pixelIndex = 0; pixelIndex < rgba.size() / 4U; ++pixelIndex) {
    const auto offset = pixelIndex * 4U;
    if(rgba[offset + 0] != background[0] || rgba[offset + 1] != background[1]
       || rgba[offset + 2] != background[2] || rgba[offset + 3] != background[3]) {
      return true;
    }
  }

  return false;
}

std::vector<LoadedChart> loadReadableCharts(const std::filesystem::path &root)
{
  chart_view::runtime::s57::S57Reader reader;
  std::vector<LoadedChart> loadedCharts;

  for(const auto &chartPath : findS57Charts(root)) {
    auto readResult = reader.read(chartPath.string());
    if(!readResult.ok || readResult.dataset.empty() || !readResult.dataset.meta().extent.isValid()) {
      continue;
    }

    auto prepared = prepareDatasetForSmoke(chartPath, readResult.dataset);
    loadedCharts.push_back({chartPath, std::move(readResult), std::move(prepared)});
  }

  return loadedCharts;
}
}// namespace

TEST_CASE("S57 real-chart smoke validates the broader Phase 5 pipeline on available multi-scale samples",
          "[s57][phase5][real][smoke][rhi]")
{
  AppGuard guard;

  const auto root = getS57Root();
  auto loadedCharts = loadReadableCharts(root);
  if(loadedCharts.size() < 3U) {
    SKIP("Need at least three readable S57 charts for the broader Phase 5 real-chart smoke");
  }

  const auto chartA = std::find_if(
    loadedCharts.begin(),
    loadedCharts.end(),
    [](const LoadedChart &chart) { return chart.sourcePath.stem().string() == kTargetChartAStem; });
  const auto chartB = std::find_if(
    loadedCharts.begin(),
    loadedCharts.end(),
    [](const LoadedChart &chart) { return chart.sourcePath.stem().string() == kTargetChartBStem; });
  if(chartA == loadedCharts.end() || chartB == loadedCharts.end()) {
    SKIP("Known fixed-pair charts C1511781/C1511782 not available for Phase 5 real-chart smoke");
  }

  std::vector<const LoadedChart *> selectedCharts{&(*chartA), &(*chartB)};
  std::unordered_set<std::string> selectedIds{
    chartA->preparedDataset.meta().name,
    chartB->preparedDataset.meta().name};
  std::unordered_set<std::uint32_t> selectedBands{
    chartA->preparedDataset.meta().usageBand,
    chartB->preparedDataset.meta().usageBand};

  for(const auto &chart : loadedCharts) {
    if(selectedIds.contains(chart.preparedDataset.meta().name)) {
      continue;
    }

    if(selectedBands.insert(chart.preparedDataset.meta().usageBand).second) {
      selectedCharts.push_back(&chart);
      selectedIds.insert(chart.preparedDataset.meta().name);
    }
  }

  if(selectedCharts.size() < 3U) {
    for(const auto &chart : loadedCharts) {
      if(selectedIds.contains(chart.preparedDataset.meta().name)) {
        continue;
      }
      selectedCharts.push_back(&chart);
      selectedIds.insert(chart.preparedDataset.meta().name);
      if(selectedCharts.size() >= 3U) {
        break;
      }
    }
  }

  if(selectedCharts.size() < 3U) {
    SKIP("Need fixed pair plus at least one additional readable S57 chart for Phase 5 real-chart smoke");
  }

  chart_view::runtime::portrayal::S52DisplaySettings settings;
  settings.pointSymbolMode = chart_view::runtime::portrayal::S52PointSymbolMode::kSimplified;
  chart_view::runtime::portrayal::FeatureSymbolizer symbolizer(settings);

  std::size_t totalS52Hits = 0;
  std::size_t totalNamed = 0;
  std::size_t totalUnicodeNamed = 0;
  std::size_t totalTextCandidates = 0;
  std::size_t totalVisibleLabels = 0;

  for(const auto *chart : selectedCharts) {
    INFO("chart=" << chart->preparedDataset.meta().name
                  << " usageBand=" << chart->preparedDataset.meta().usageBand
                  << " nativeScale=" << chart->preparedDataset.meta().nativeScale);

    chart_view::runtime::senc::SencWriter writer;
    writer.setFormatVersion(chart_view::runtime::senc::kSencFormatVersionV2);
    writer.setSourceManifest(chart->readResult.sourceModel.sourceManifest);
    writer.setS57SourceModel(chart->readResult.sourceModel);
    const auto sencBlob = writer.write(chart->preparedDataset);
    REQUIRE_FALSE(sencBlob.empty());

    chart_view::runtime::senc::SencReader reader;
    const auto readback = reader.read(sencBlob);
    REQUIRE(readback.ok);
    REQUIRE(readback.sourceModel.has_value());
    REQUIRE(readback.dataset.featureCount() == chart->preparedDataset.featureCount());

    ViewportState viewportState;
    viewportState.set(makeViewportForDataset(readback.dataset));

    chart_view::runtime::SceneBuilderFromSenc builder;
    const auto snapshot = builder.buildAll(readback.dataset, viewportState);
    REQUIRE(snapshot != nullptr);
    REQUIRE_FALSE(snapshot->empty());

    chart_view::runtime::RhiRenderBackend backend;
    REQUIRE(backend.initialize(
              viewportState.viewport().pixel_width,
              viewportState.viewport().pixel_height)
            == chart_view_status_ok);

    chart_view::runtime::FeatureLayerRenderer renderer(settings);
    const auto renderResult = renderer.render(*snapshot, readback.dataset, backend);
    REQUIRE(renderResult.status == chart_view_status_ok);
    REQUIRE(renderResult.totalVertices > 0U);

    std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
    REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);
    REQUIRE(frameHasNonBackgroundPixel(rgba));

    chart_view::runtime::projection::ProjectionContext projectionContext =
      chart_view::runtime::projection::ProjectionContext::createMercator();
    REQUIRE(projectionContext.isValid());

    chart_view::runtime::projection::ProjectedViewport projectedViewport{};
    REQUIRE(chart_view::runtime::projection::ProjectedViewport::create(
      viewportState.viewport(),
      projectionContext,
      projectedViewport));

    const auto visibleLabelStats = collectVisibleProjectedLabelStats(
      *snapshot,
      readback.dataset,
      symbolizer,
      renderer.portrayalRegistry(),
      projectionContext,
      projectedViewport);

    const auto s52Hits = countS52SymbolizedFeatures(readback.dataset, symbolizer);
    const auto named = countNamedFeatures(readback.dataset);
    const auto unicodeNamed = countUnicodeNamedFeatures(readback.dataset);
    const auto textCandidates = countTextLabelCandidates(readback.dataset, symbolizer);

    std::cout << "real-chart sample: " << readback.dataset.meta().name
              << " usageBand=" << readback.dataset.meta().usageBand
              << " features=" << readback.dataset.featureCount()
              << " s52Hits=" << s52Hits
              << " named=" << named
              << " unicodeNamed=" << unicodeNamed
              << " textCandidates=" << textCandidates
              << " visibleLabels=" << visibleLabelStats.totalVisibleLabels
              << " visibleUnicodeLabels=" << visibleLabelStats.visibleUnicodeLabels
              << std::endl;

    REQUIRE(s52Hits > 0U);
    totalS52Hits += s52Hits;
    totalNamed += named;
    totalUnicodeNamed += unicodeNamed;
    totalTextCandidates += textCandidates;
    totalVisibleLabels += visibleLabelStats.totalVisibleLabels;
  }

  REQUIRE(selectedIds.contains(std::string(kTargetChartAStem)));
  REQUIRE(selectedIds.contains(std::string(kTargetChartBStem)));
  REQUIRE(totalS52Hits > 0U);
  REQUIRE(totalNamed > 0U);
  REQUIRE(totalUnicodeNamed > 0U);
  REQUIRE(totalTextCandidates > 0U);
  REQUIRE(totalVisibleLabels > 0U);
}
