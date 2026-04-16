#include <catch2/catch_test_macros.hpp>

#include "catalog/chart_catalog.hpp"
#include "catalog/chart_selection_policy.hpp"
#include "catalog/coverage_index.hpp"
#include "feature_layer_renderer.hpp"
#include "label_layout.hpp"
#include "portrayal/feature_symbolizer.hpp"
#include "portrayal/s52_display_settings.hpp"
#include "projection/projected_bounds.hpp"
#include "quilt/quilt_planner.hpp"
#include "quilt/zoom_policy.hpp"
#include "rhi_render_backend.hpp"
#include "scene_builder_from_senc.hpp"
#include "s57/s57_reader.hpp"
#include "senc/senc_reader.hpp"
#include "senc/senc_writer.hpp"
#include "text_label_renderer.hpp"

#include <QByteArray>
#include <QGuiApplication>
#include <QtGlobal>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {
using chart_view::runtime::ViewportState;
using chart_view::runtime::chart_data::DatasetMeta;
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
char *AppGuard::argv[] = {const_cast<char *>("s57_quilt_smoke_tests"), nullptr};

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
  std::size_t s52FeatureCount{0};
  std::size_t namedFeatureCount{0};
  std::size_t unicodeNamedFeatureCount{0};
  std::size_t textLabelCandidateCount{0};
};

constexpr std::string_view kTargetChartAStem = "C1511781";
constexpr std::string_view kTargetChartBStem = "C1511782";

enum class ExtentRelation
{
  kDisjoint,
  kAdjacent,
  kOverlap,
};

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

std::filesystem::path findS57ChartByStem(
  const std::filesystem::path &root,
  std::string_view stem)
{
  const auto charts = findS57Charts(root);
  const auto it = std::find_if(
    charts.begin(),
    charts.end(),
    [&](const auto &chartPath) { return chartPath.stem().string() == stem; });
  return it == charts.end() ? std::filesystem::path{} : *it;
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

bool extentsOverlapOrTouch(const Extent &lhs, const Extent &rhs) noexcept
{
  return lhs.minLon <= rhs.maxLon && lhs.maxLon >= rhs.minLon && lhs.minLat <= rhs.maxLat
      && lhs.maxLat >= rhs.minLat;
}

ExtentRelation classifyExtentRelation(const Extent &lhs, const Extent &rhs) noexcept
{
  if(!lhs.isValid() || !rhs.isValid()) {
    return ExtentRelation::kDisjoint;
  }

  const auto overlapMinLon = std::max(lhs.minLon, rhs.minLon);
  const auto overlapMaxLon = std::min(lhs.maxLon, rhs.maxLon);
  const auto overlapMinLat = std::max(lhs.minLat, rhs.minLat);
  const auto overlapMaxLat = std::min(lhs.maxLat, rhs.maxLat);

  if(overlapMinLon > overlapMaxLon || overlapMinLat > overlapMaxLat) {
    return ExtentRelation::kDisjoint;
  }

  if(overlapMinLon < overlapMaxLon && overlapMinLat < overlapMaxLat) {
    return ExtentRelation::kOverlap;
  }

  return ExtentRelation::kAdjacent;
}

ExtentRelation classifyExtentRelation(
  const chart_view::runtime::projection::ProjectedExtent &lhs,
  const chart_view::runtime::projection::ProjectedExtent &rhs) noexcept
{
  if(!lhs.isValid() || !rhs.isValid()) {
    return ExtentRelation::kDisjoint;
  }

  const auto overlapMinX = std::max(lhs.minX, rhs.minX);
  const auto overlapMaxX = std::min(lhs.maxX, rhs.maxX);
  const auto overlapMinY = std::max(lhs.minY, rhs.minY);
  const auto overlapMaxY = std::min(lhs.maxY, rhs.maxY);

  if(overlapMinX > overlapMaxX || overlapMinY > overlapMaxY) {
    return ExtentRelation::kDisjoint;
  }

  if(overlapMinX < overlapMaxX && overlapMinY < overlapMaxY) {
    return ExtentRelation::kOverlap;
  }

  return ExtentRelation::kAdjacent;
}

Extent unionExtents(const Extent &lhs, const Extent &rhs) noexcept
{
  return {
    std::min(lhs.minLon, rhs.minLon),
    std::min(lhs.minLat, rhs.minLat),
    std::max(lhs.maxLon, rhs.maxLon),
    std::max(lhs.maxLat, rhs.maxLat)};
}

std::string formatExtent(const Extent &extent)
{
  std::ostringstream stream;
  stream << "[" << extent.minLon << ", " << extent.minLat << "] -> [" << extent.maxLon << ", "
         << extent.maxLat << "]";
  return stream.str();
}

std::string formatProjectedExtent(
  const chart_view::runtime::projection::ProjectedExtent &extent)
{
  std::ostringstream stream;
  stream << "[" << extent.minX << ", " << extent.minY << "] -> [" << extent.maxX << ", "
         << extent.maxY << "]";
  return stream.str();
}

const char *toString(ExtentRelation relation) noexcept
{
  switch(relation) {
  case ExtentRelation::kOverlap:
    return "overlap";
  case ExtentRelation::kAdjacent:
    return "adjacent";
  case ExtentRelation::kDisjoint:
  default:
    return "disjoint";
  }
}

double projectedArea(const chart_view::runtime::projection::ProjectedExtent &extent) noexcept
{
  if(!extent.isValid()) {
    return 0.0;
  }

  return std::max(0.0, extent.maxX - extent.minX) * std::max(0.0, extent.maxY - extent.minY);
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

void requireProjectedQuiltPlan(const chart_view::runtime::quilt::QuiltPlan &plan)
{
  REQUIRE(plan.projectedViewportExtent().isValid());
  REQUIRE(plan.selectionResult().candidateCount >= plan.layers().size());

  for(const auto &layer : plan.layers()) {
    REQUIRE(layer.projectedFullExtent.isValid());
    REQUIRE(layer.projectedVisibleExtent.isValid());
    REQUIRE_FALSE(layer.projectedPatchExtents.empty());

    for(const auto &patch : layer.projectedPatchExtents) {
      REQUIRE(patch.isValid());
      REQUIRE(patch.minX <= plan.projectedViewportExtent().maxX);
      REQUIRE(patch.maxX >= plan.projectedViewportExtent().minX);
      REQUIRE(patch.minY <= plan.projectedViewportExtent().maxY);
      REQUIRE(patch.maxY >= plan.projectedViewportExtent().minY);
    }
  }
}

bool frameHasNonBackgroundPixel(
  std::span<const std::uint8_t> rgba,
  const std::array<std::uint8_t, 4> &background)
{
  for(std::size_t pixelIndex = 0; pixelIndex < rgba.size() / 4U; ++pixelIndex) {
    const auto offset = pixelIndex * 4U;
    if(rgba[offset + 0] != background[0] || rgba[offset + 1] != background[1]
       || rgba[offset + 2] != background[2] || rgba[offset + 3] != background[3]) {
      return true;
    }
  }

  return false;
}

std::size_t countS52SymbolizedFeatures(
  std::span<const FeatureChartDataset> datasets,
  const chart_view::runtime::portrayal::FeatureSymbolizer &symbolizer)
{
  std::size_t count = 0;
  for(const auto &dataset : datasets) {
    for(const auto &feature : dataset.features()) {
      if(symbolizer.symbolize(feature).s52Lookup.has_value()) {
        ++count;
      }
    }
  }

  return count;
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

std::size_t countRawStringAttribute(
  const FeatureChartDataset &dataset,
  std::string_view key)
{
  std::size_t count = 0;
  for(const auto &feature : dataset.features()) {
    const auto it = feature.attributes.find(std::string(key));
    if(it == feature.attributes.end()) {
      continue;
    }

    const auto *value = std::get_if<std::string>(&it->second);
    if(value != nullptr && !value->empty()) {
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

std::optional<chart_view::runtime::LabelItem> findExpectedRenderedLabel(
  const chart_view::runtime::SceneSnapshot &snapshot,
  std::span<const FeatureChartDataset> datasets,
  const chart_view::runtime::portrayal::FeatureSymbolizer &symbolizer,
  const chart_view::runtime::portrayal::PortrayalRegistry &registry,
  const chart_view::runtime::projection::ProjectionContext &projectionContext,
  const chart_view::runtime::projection::ProjectedViewport &projectedViewport)
{
  chart_view::runtime::TextLabelRenderer labelRenderer;
  std::vector<chart_view::runtime::label::LabelBounds> occupiedBounds;
  std::optional<chart_view::runtime::LabelItem> firstAcceptedLabel;

  for(const auto &entry : snapshot.layers()) {
    if(entry.sourceChartIndex >= datasets.size()) {
      continue;
    }

    const auto &features = datasets[entry.sourceChartIndex].features();
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
    auto label = labelRenderer.layout(symbolization.textKey, feature, anchor, rule);
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
    if(hasNonAsciiGlyph(*label)) {
      return label;
    }

    if(!firstAcceptedLabel.has_value()) {
      firstAcceptedLabel = label;
    }
  }

  return firstAcceptedLabel;
}

struct VisibleLabelAuditStats
{
  std::size_t totalVisibleLabels{0};
  std::size_t visibleUnicodeLabels{0};
};

VisibleLabelAuditStats collectVisibleProjectedLabelStats(
  const chart_view::runtime::SceneSnapshot &snapshot,
  std::span<const FeatureChartDataset> datasets,
  const chart_view::runtime::portrayal::FeatureSymbolizer &symbolizer,
  const chart_view::runtime::portrayal::PortrayalRegistry &registry,
  const chart_view::runtime::projection::ProjectionContext &projectionContext,
  const chart_view::runtime::projection::ProjectedViewport &projectedViewport)
{
  chart_view::runtime::TextLabelRenderer labelRenderer;
  std::vector<chart_view::runtime::label::LabelBounds> occupiedBounds;
  VisibleLabelAuditStats stats;

  for(const auto &entry : snapshot.layers()) {
    if(entry.sourceChartIndex >= datasets.size()) {
      continue;
    }

    const auto &features = datasets[entry.sourceChartIndex].features();
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
    auto label = labelRenderer.layout(symbolization.textKey, feature, anchor, rule);
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

std::string joinChartIds(std::span<const std::string> ids)
{
  std::ostringstream stream;
  for(std::size_t i = 0; i < ids.size(); ++i) {
    if(i > 0) {
      stream << ", ";
    }
    stream << ids[i];
  }
  return stream.str();
}

template<typename T>
std::string joinChartEntryIds(const std::vector<const T *> &entries)
{
  std::ostringstream stream;
  for(std::size_t i = 0; i < entries.size(); ++i) {
    if(i > 0) {
      stream << ", ";
    }
    stream << (entries[i] != nullptr ? entries[i]->id : "<null>");
  }
  return stream.str();
}

bool regionHasColor(
  std::span<const std::uint8_t> rgba,
  int width,
  int height,
  const chart_view::runtime::label::LabelBounds &bounds,
  const std::array<std::uint8_t, 4> &color)
{
  for(int y = (std::max)(0, bounds.top); y <= (std::min)(height - 1, bounds.bottom); ++y) {
    for(int x = (std::max)(0, bounds.left); x <= (std::min)(width - 1, bounds.right); ++x) {
      const auto offset = static_cast<std::size_t>((y * width + x) * 4);
      if(offset + 3 >= rgba.size()) {
        continue;
      }

      if(rgba[offset + 0] == color[0] && rgba[offset + 1] == color[1]
         && rgba[offset + 2] == color[2] && rgba[offset + 3] == color[3]) {
        return true;
      }
    }
  }

  return false;
}

std::array<LoadedChart, 2> selectBestPair(const std::filesystem::path &root)
{
  const auto chartFiles = findS57Charts(root);
  if(chartFiles.size() < 2) {
    SKIP("Need at least two S57 .000 files for quilt smoke");
  }

  chart_view::runtime::portrayal::S52DisplaySettings settings;
  settings.pointSymbolMode = chart_view::runtime::portrayal::S52PointSymbolMode::kSimplified;
  chart_view::runtime::portrayal::FeatureSymbolizer symbolizer(settings);
  chart_view::runtime::s57::S57Reader reader;
  std::vector<LoadedChart> loadedCharts;
  loadedCharts.reserve(chartFiles.size());

  for(const auto &chartPath : chartFiles) {
    const auto result = reader.read(chartPath.string());
    if(!result.ok || result.dataset.empty() || !result.dataset.meta().extent.isValid()) {
      continue;
    }

    auto dataset = prepareDatasetForSmoke(chartPath, result.dataset);
    loadedCharts.push_back(LoadedChart{
      chartPath,
      dataset,
      countS52SymbolizedFeatures(dataset, symbolizer),
      countNamedFeatures(dataset),
      countUnicodeNamedFeatures(dataset),
      countTextLabelCandidates(dataset, symbolizer)});
  }

  if(loadedCharts.size() < 2) {
    SKIP("Need two readable S57 charts with valid extents for quilt smoke");
  }

  std::size_t bestLeft = 0;
  std::size_t bestRight = 0;
  auto bestUnionArea = std::numeric_limits<double>::max();
  std::size_t bestS52FeatureCount = 0;
  std::size_t bestNamedFeatureCount = 0;
  std::size_t bestUnicodeNamedFeatureCount = 0;
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
      const auto combinedS52FeatureCount =
        loadedCharts[left].s52FeatureCount + loadedCharts[right].s52FeatureCount;
      const auto combinedNamedFeatureCount =
        loadedCharts[left].namedFeatureCount + loadedCharts[right].namedFeatureCount;
      const auto combinedUnicodeNamedFeatureCount =
        loadedCharts[left].unicodeNamedFeatureCount + loadedCharts[right].unicodeNamedFeatureCount;

      const auto betterPair =
        !foundPair
        || combinedS52FeatureCount > bestS52FeatureCount
        || (combinedS52FeatureCount == bestS52FeatureCount
            && combinedUnicodeNamedFeatureCount > bestUnicodeNamedFeatureCount)
        || (combinedS52FeatureCount == bestS52FeatureCount
            && combinedUnicodeNamedFeatureCount == bestUnicodeNamedFeatureCount
            && combinedNamedFeatureCount > bestNamedFeatureCount)
        || (combinedS52FeatureCount == bestS52FeatureCount
            && combinedUnicodeNamedFeatureCount == bestUnicodeNamedFeatureCount
            && combinedNamedFeatureCount == bestNamedFeatureCount
            && unionArea < bestUnionArea);
      if(betterPair) {
        bestUnionArea = unionArea;
        bestLeft = left;
        bestRight = right;
        bestS52FeatureCount = combinedS52FeatureCount;
        bestNamedFeatureCount = combinedNamedFeatureCount;
        bestUnicodeNamedFeatureCount = combinedUnicodeNamedFeatureCount;
        foundPair = true;
      }
    }
  }

  if(!foundPair) {
    SKIP("No overlapping or adjacent S57 chart pair found for quilt smoke");
  }

  if(bestS52FeatureCount == 0U) {
    SKIP("No overlapping S57 chart pair with S-52 baseline lookup coverage found for integrated smoke");
  }

  return {loadedCharts[bestLeft], loadedCharts[bestRight]};
}
}// namespace

TEST_CASE(
  "Targeted pair audit for known overlapping S57 charts C1511781 and C1511782",
  "[s57][quilt][projection][smoke][real-data][audit][targeted-pair]")
{
  AppGuard guard;

  const auto root = getS57Root();
  const auto chartAPath = findS57ChartByStem(root, kTargetChartAStem);
  const auto chartBPath = findS57ChartByStem(root, kTargetChartBStem);
  REQUIRE_FALSE(chartAPath.empty());
  REQUIRE_FALSE(chartBPath.empty());

  const auto discoveredCharts = findS57Charts(root);
  const auto chartADiscovered = std::find(discoveredCharts.begin(), discoveredCharts.end(), chartAPath)
                             != discoveredCharts.end();
  const auto chartBDiscovered = std::find(discoveredCharts.begin(), discoveredCharts.end(), chartBPath)
                             != discoveredCharts.end();

  chart_view::runtime::portrayal::S52DisplaySettings settings;
  settings.pointSymbolMode = chart_view::runtime::portrayal::S52PointSymbolMode::kSimplified;
  chart_view::runtime::portrayal::FeatureSymbolizer symbolizer(settings);
  chart_view::runtime::s57::S57Reader reader;

  const auto chartARead = reader.read(chartAPath.string());
  const auto chartBRead = reader.read(chartBPath.string());
  REQUIRE(chartARead.ok);
  REQUIRE(chartBRead.ok);
  REQUIRE_FALSE(chartARead.dataset.empty());
  REQUIRE_FALSE(chartBRead.dataset.empty());
  REQUIRE(chartARead.dataset.meta().extent.isValid());
  REQUIRE(chartBRead.dataset.meta().extent.isValid());

  const LoadedChart chartA{
    chartAPath,
    prepareDatasetForSmoke(chartAPath, chartARead.dataset),
    countS52SymbolizedFeatures(prepareDatasetForSmoke(chartAPath, chartARead.dataset), symbolizer),
    countNamedFeatures(prepareDatasetForSmoke(chartAPath, chartARead.dataset)),
    countUnicodeNamedFeatures(prepareDatasetForSmoke(chartAPath, chartARead.dataset)),
    countTextLabelCandidates(prepareDatasetForSmoke(chartAPath, chartARead.dataset), symbolizer)};
  const LoadedChart chartB{
    chartBPath,
    prepareDatasetForSmoke(chartBPath, chartBRead.dataset),
    countS52SymbolizedFeatures(prepareDatasetForSmoke(chartBPath, chartBRead.dataset), symbolizer),
    countNamedFeatures(prepareDatasetForSmoke(chartBPath, chartBRead.dataset)),
    countUnicodeNamedFeatures(prepareDatasetForSmoke(chartBPath, chartBRead.dataset)),
    countTextLabelCandidates(prepareDatasetForSmoke(chartBPath, chartBRead.dataset), symbolizer)};

  const auto geographicRelation =
    classifyExtentRelation(chartA.dataset.meta().extent, chartB.dataset.meta().extent);

  const auto projectionContext = chart_view::runtime::projection::ProjectionContext::createMercator();
  REQUIRE(projectionContext.isValid());

  chart_view::runtime::projection::ProjectedExtent projectedExtentA{};
  chart_view::runtime::projection::ProjectedExtent projectedExtentB{};
  REQUIRE(projectionContext.projectExtent(chartA.dataset.meta().extent, projectedExtentA));
  REQUIRE(projectionContext.projectExtent(chartB.dataset.meta().extent, projectedExtentB));
  const auto projectedRelation = classifyExtentRelation(projectedExtentA, projectedExtentB);

  const auto tempPath =
    std::filesystem::temp_directory_path() / "chart_view_s57_targeted_pair_audit";
  {
    std::error_code ec;
    std::filesystem::remove_all(tempPath, ec);
  }
  REQUIRE(std::filesystem::create_directories(tempPath));
  TempDirGuard tempDir(tempPath);

  chart_view::runtime::senc::SencWriter writer;
  for(const auto *chart : {&chartA, &chartB}) {
    const auto blob = writer.write(chart->dataset);
    REQUIRE_FALSE(blob.empty());
    writeBlob(tempDir.path / (chart->dataset.meta().name + ".senc"), blob);
  }

  chart_view::runtime::catalog::ChartCatalog catalog;
  REQUIRE(catalog.loadDirectory(tempDir.path));
  const auto *catalogEntryA = catalog.findById(std::string(kTargetChartAStem));
  const auto *catalogEntryB = catalog.findById(std::string(kTargetChartBStem));
  REQUIRE(catalogEntryA != nullptr);
  REQUIRE(catalogEntryB != nullptr);

  chart_view::runtime::catalog::CoverageIndex coverageIndex;
  REQUIRE(coverageIndex.build(catalog));

  const auto combinedExtent = unionExtents(chartA.dataset.meta().extent, chartB.dataset.meta().extent);
  constexpr int kViewportWidth = 1600;
  constexpr int kViewportHeight = 900;
  const auto baseScale = estimateScaleForExtent(combinedExtent, kViewportWidth, kViewportHeight);
  const auto baseViewport = makeViewport(combinedExtent, baseScale, kViewportWidth, kViewportHeight);

  Extent baseViewportExtent{};
  REQUIRE(chart_view::runtime::projection::computeViewportGeographicExtent(
    baseViewport,
    projectionContext,
    baseViewportExtent));

  const auto coverageCandidates = coverageIndex.query(baseViewportExtent);
  chart_view::runtime::catalog::ChartSelectionPolicy selectionPolicy;
  const auto rankedCandidates =
    selectionPolicy.rankCandidates(coverageCandidates, baseViewport.scale_denominator);

  chart_view::runtime::quilt::QuiltPlanner planner;
  const auto basePlan =
    planner.build(coverageIndex, selectionPolicy, baseViewportExtent, baseViewport.scale_denominator);
  REQUIRE_FALSE(basePlan.empty());

  const auto quiltIncludesBoth = std::count_if(
    basePlan.layers().begin(),
    basePlan.layers().end(),
    [](const auto &layer) {
      return layer.chartId == kTargetChartAStem || layer.chartId == kTargetChartBStem;
    })
    == 2;
  const auto chartALayerIt = std::find_if(
    basePlan.layers().begin(),
    basePlan.layers().end(),
    [](const auto &layer) { return layer.chartId == kTargetChartAStem; });
  const auto chartBLayerIt = std::find_if(
    basePlan.layers().begin(),
    basePlan.layers().end(),
    [](const auto &layer) { return layer.chartId == kTargetChartBStem; });
  auto patchAreaSumForLayer = [](const chart_view::runtime::quilt::QuiltLayer &layer) {
    double patchAreaSum = 0.0;
    for(const auto &patch : layer.projectedPatchExtents) {
      patchAreaSum += projectedArea(patch);
    }
    return patchAreaSum;
  };
  auto clippedVisibleArea = [&](const chart_view::runtime::quilt::QuiltLayer &layer) {
    return patchAreaSumForLayer(layer) + 1.0 < projectedArea(layer.projectedVisibleExtent);
  };

  REQUIRE(basePlan.layers().size() == 2);
  REQUIRE(quiltIncludesBoth);
  REQUIRE(chartALayerIt != basePlan.layers().end());
  REQUIRE(chartBLayerIt != basePlan.layers().end());
  REQUIRE_FALSE(chartALayerIt->projectedPatchExtents.empty());
  REQUIRE_FALSE(chartBLayerIt->projectedPatchExtents.empty());
  REQUIRE(patchAreaSumForLayer(*chartALayerIt) > 0.0);
  REQUIRE(patchAreaSumForLayer(*chartBLayerIt) > 0.0);
  const bool anyLayerWasClipped =
    clippedVisibleArea(*chartALayerIt) || clippedVisibleArea(*chartBLayerIt);
  REQUIRE(anyLayerWasClipped);

  const auto baseDatasets = loadPlanDatasets(basePlan);
  REQUIRE_FALSE(baseDatasets.empty());

  ViewportState viewportState;
  viewportState.set(baseViewport);

  chart_view::runtime::SceneBuilderFromSenc builder;
  const auto snapshot =
    builder.build(basePlan, std::span<const FeatureChartDataset>(baseDatasets), viewportState);
  REQUIRE(snapshot != nullptr);

  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE(backend.initialize(kViewportWidth, kViewportHeight) == chart_view_status_ok);

  chart_view::runtime::FeatureLayerRenderer renderer(settings);
  const std::array<std::uint8_t, 4> labelColor{17U, 231U, 133U, 255U};
  renderer.portrayalRegistry().registerTextRuleForStyle(
    "text/default",
    chart_view::runtime::portrayal::TextRule{labelColor, 12U});
  const auto renderResult = renderer.render(
    *snapshot,
    std::span<const FeatureChartDataset>(baseDatasets),
    backend);
  REQUIRE(renderResult.status == chart_view_status_ok);

  chart_view::runtime::projection::ProjectedViewport projectedViewport;
  REQUIRE(chart_view::runtime::projection::ProjectedViewport::create(
    baseViewport,
    projectionContext,
    projectedViewport));

  const auto visibleLabelStats = collectVisibleProjectedLabelStats(
    *snapshot,
    std::span<const FeatureChartDataset>(baseDatasets),
    symbolizer,
    renderer.portrayalRegistry(),
    projectionContext,
    projectedViewport);

  std::cout << "=== targeted pair audit ===\n";
  std::cout << "root: " << root.string() << "\n";
  std::cout << "chart A path: " << chartAPath.string() << "\n";
  std::cout << "chart B path: " << chartBPath.string() << "\n";
  std::cout << "raw directory discovery: A=" << chartADiscovered << " B=" << chartBDiscovered
            << " total=" << discoveredCharts.size() << "\n";
  std::cout << "chart A meta: name=" << chartA.dataset.meta().name
            << " usageBand=" << chartA.dataset.meta().usageBand
            << " nativeScale=" << chartA.dataset.meta().nativeScale
            << " extent=" << formatExtent(chartA.dataset.meta().extent)
            << " features=" << chartA.dataset.featureCount()
            << " named=" << chartA.namedFeatureCount
            << " unicodeNamed=" << chartA.unicodeNamedFeatureCount
            << " textCandidates=" << chartA.textLabelCandidateCount
            << " s52Hits=" << chartA.s52FeatureCount << "\n";
  std::cout << "chart B meta: name=" << chartB.dataset.meta().name
            << " usageBand=" << chartB.dataset.meta().usageBand
            << " nativeScale=" << chartB.dataset.meta().nativeScale
            << " extent=" << formatExtent(chartB.dataset.meta().extent)
            << " features=" << chartB.dataset.featureCount()
            << " named=" << chartB.namedFeatureCount
            << " unicodeNamed=" << chartB.unicodeNamedFeatureCount
            << " textCandidates=" << chartB.textLabelCandidateCount
            << " s52Hits=" << chartB.s52FeatureCount << "\n";
  std::cout << "geographic relation: " << toString(geographicRelation) << "\n";
  std::cout << "projected extent A: " << formatProjectedExtent(projectedExtentA) << "\n";
  std::cout << "projected extent B: " << formatProjectedExtent(projectedExtentB) << "\n";
  std::cout << "projected relation: " << toString(projectedRelation) << "\n";
  std::cout << "base viewport scale: " << baseViewport.scale_denominator << "\n";
  std::cout << "catalog size: " << catalog.size()
            << " findById(A)=" << (catalogEntryA != nullptr)
            << " findById(B)=" << (catalogEntryB != nullptr) << "\n";
  std::cout << "coverage candidates: " << joinChartEntryIds(coverageCandidates) << "\n";
  std::cout << "ranked candidates: " << joinChartEntryIds(rankedCandidates) << "\n";
  std::cout << "quilt ordered chart ids: "
            << joinChartIds(std::span<const std::string>(basePlan.selectionResult().orderedChartIds))
            << "\n";
  std::cout << "quilt includes both target charts: " << quiltIncludesBoth << "\n";

  for(const auto &layer : basePlan.layers()) {
    const auto patchAreaSum = patchAreaSumForLayer(layer);

    std::cout << "layer " << layer.chartId << ": drawOrder=" << layer.drawOrder
              << " fullExtent=" << formatExtent(layer.fullExtent)
              << " visibleExtent=" << formatExtent(layer.visibleExtent)
              << " projectedVisible=" << formatProjectedExtent(layer.projectedVisibleExtent)
              << " patchCount=" << layer.projectedPatchExtents.size()
              << " visibleArea=" << projectedArea(layer.projectedVisibleExtent)
              << " patchAreaSum=" << patchAreaSum
              << " clipped=" << clippedVisibleArea(layer)
              << "\n";
  }

  std::cout << "snapshot charts=" << snapshot->charts().size()
            << " layers=" << snapshot->layers().size() << "\n";
  std::cout << "render totals: vertices=" << renderResult.totalVertices
            << " points=" << renderResult.pointsRendered
            << " lines=" << renderResult.linesRendered
            << " areas=" << renderResult.areasRendered << "\n";
  std::cout << "visible projected labels: total=" << visibleLabelStats.totalVisibleLabels
            << " unicode=" << visibleLabelStats.visibleUnicodeLabels << "\n";

  const auto combinedS52Hits = chartA.s52FeatureCount + chartB.s52FeatureCount;
  const auto combinedNamedFeatures = chartA.namedFeatureCount + chartB.namedFeatureCount;
  const auto combinedUnicodeNames =
    chartA.unicodeNamedFeatureCount + chartB.unicodeNamedFeatureCount;
  const auto combinedTextCandidates =
    chartA.textLabelCandidateCount + chartB.textLabelCandidateCount;
  const auto combinedRawNationalNames =
    countRawStringAttribute(chartA.dataset, "NOBJNM") + countRawStringAttribute(chartB.dataset, "NOBJNM");
  std::string rootCause;
  if(!chartADiscovered || !chartBDiscovered || catalogEntryA == nullptr || catalogEntryB == nullptr) {
    rootCause = "pair discovery/catalog inclusion failure";
  } else if(geographicRelation == ExtentRelation::kDisjoint) {
    rootCause = "geographic coverage relation is disjoint in current runtime extents";
  } else if(projectedRelation == ExtentRelation::kDisjoint) {
    rootCause = "projected coverage relation is disjoint";
  } else if(!quiltIncludesBoth) {
    rootCause =
      combinedS52Hits == 0U || combinedTextCandidates == 0U
        ? "quilt plan excludes chart B after ranking/patch clipping, and the parsed pair also has zero S-52 baseline hits / label candidates"
        : "quilt plan excludes chart B after ranking/patch clipping";
  } else if(combinedS52Hits == 0U) {
    rootCause =
      "both charts reach quilt, but real parse leaves zero S-52 baseline lookup hits";
  } else if(combinedNamedFeatures == 0U || combinedTextCandidates == 0U) {
    rootCause =
      "both charts reach quilt, but real parse leaves zero label candidate attributes";
  } else if(visibleLabelStats.totalVisibleLabels == 0U) {
    rootCause = "labels exist semantically but none survive projected layout";
  } else {
    rootCause = "targeted pair currently reaches the integrated projected quilt + S-52 + label path";
  }

  std::cout << "combined counts: s52Hits=" << combinedS52Hits
            << " named=" << combinedNamedFeatures
            << " unicodeNamed=" << combinedUnicodeNames
            << " textCandidates=" << combinedTextCandidates
            << " rawNobjnm=" << combinedRawNationalNames << "\n";
  std::cout << "most likely root cause: " << rootCause << "\n";

  REQUIRE(combinedS52Hits > 0U);
  REQUIRE(combinedNamedFeatures > 0U);
  REQUIRE(combinedTextCandidates > 0U);
}

TEST_CASE(
  "Projected S57 quilt smoke renders and zooms two real charts",
  "[s57][quilt][projection][smoke][real-data][rhi]")
{
  AppGuard guard;

  const auto root = getS57Root();
  const auto charts = selectBestPair(root);

  INFO("chart A: " << charts[0].sourcePath.string());
  INFO("chart B: " << charts[1].sourcePath.string());

  const auto tempPath = std::filesystem::temp_directory_path() / "chart_view_s57_quilt_smoke";
  {
    std::error_code ec;
    std::filesystem::remove_all(tempPath, ec);
  }
  REQUIRE(std::filesystem::create_directories(tempPath));
  TempDirGuard tempDir(tempPath);

  chart_view::runtime::senc::SencWriter writer;
  for(const auto &chart : charts) {
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
    unionExtents(charts[0].dataset.meta().extent, charts[1].dataset.meta().extent);
  constexpr int kViewportWidth = 1600;
  constexpr int kViewportHeight = 900;
  const auto baseScale = estimateScaleForExtent(combinedExtent, kViewportWidth, kViewportHeight);
  const auto baseViewport = makeViewport(combinedExtent, baseScale, kViewportWidth, kViewportHeight);
  const auto projectionContext = chart_view::runtime::projection::ProjectionContext::createMercator();
  REQUIRE(projectionContext.isValid());

  Extent baseViewportExtent{};
  REQUIRE(chart_view::runtime::projection::computeViewportGeographicExtent(
    baseViewport,
    projectionContext,
    baseViewportExtent));

  chart_view::runtime::catalog::ChartSelectionPolicy selectionPolicy;
  chart_view::runtime::quilt::QuiltPlanner planner;
  const auto basePlan = planner.build(
    coverageIndex,
    selectionPolicy,
    baseViewportExtent,
    baseViewport.scale_denominator);

  REQUIRE(basePlan.layers().size() == 2);
  REQUIRE(basePlan.layers()[0].sourceType == chart_view_chart_source_s57);
  REQUIRE(basePlan.layers()[1].sourceType == chart_view_chart_source_s57);
  requireProjectedQuiltPlan(basePlan);

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

  chart_view::runtime::portrayal::S52DisplaySettings settings;
  settings.pointSymbolMode = chart_view::runtime::portrayal::S52PointSymbolMode::kSimplified;
  chart_view::runtime::portrayal::FeatureSymbolizer symbolizer(settings);
  REQUIRE(countS52SymbolizedFeatures(
    std::span<const FeatureChartDataset>(baseDatasets),
    symbolizer)
          > 0U);

  chart_view::runtime::FeatureLayerRenderer renderer(settings);
  const std::array<std::uint8_t, 4> labelColor{17U, 231U, 133U, 255U};
  renderer.portrayalRegistry().registerTextRuleForStyle(
    "text/default",
    chart_view::runtime::portrayal::TextRule{labelColor, 12U});
  const auto baseRender = renderer.render(
    *baseSnapshot,
    std::span<const FeatureChartDataset>(baseDatasets),
    backend);
  REQUIRE(baseRender.status == chart_view_status_ok);
  REQUIRE(baseRender.totalVertices > 0);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);
  REQUIRE(frameHasNonBackgroundPixel(rgba, renderer.portrayalRegistry().canvasBackgroundColor()));

  chart_view::runtime::projection::ProjectedViewport projectedViewport;
  REQUIRE(chart_view::runtime::projection::ProjectedViewport::create(
    baseViewport,
    projectionContext,
    projectedViewport));

  if(const auto expectedLabel = findExpectedRenderedLabel(
       *baseSnapshot,
       std::span<const FeatureChartDataset>(baseDatasets),
       symbolizer,
       renderer.portrayalRegistry(),
       projectionContext,
       projectedViewport);
     expectedLabel.has_value()) {
    REQUIRE(regionHasColor(
      rgba,
      kViewportWidth,
      kViewportHeight,
      expectedLabel->bounds,
      labelColor));

    if(hasNonAsciiGlyph(*expectedLabel)) {
      REQUIRE_FALSE(expectedLabel->usedPlaceholderGlyphs);
    }
  } else {
    INFO("No visible named feature survived projected label selection for this fixture pair");
  }

  chart_view::runtime::quilt::ZoomPolicy zoomPolicy;
  const auto requestedZoomScale = zoomPolicy.zoomIn(baseViewport.scale_denominator, 1);
  const auto zoomDecision = zoomPolicy.evaluate(basePlan, requestedZoomScale);
  REQUIRE(zoomDecision.resolvedScaleDenominator > 0.0);
  REQUIRE(zoomDecision.resolvedScaleDenominator < baseViewport.scale_denominator);

  auto zoomedViewport = baseViewport;
  zoomedViewport.scale_denominator = zoomDecision.resolvedScaleDenominator;

  Extent zoomedViewportExtent{};
  REQUIRE(chart_view::runtime::projection::computeViewportGeographicExtent(
    zoomedViewport,
    projectionContext,
    zoomedViewportExtent));

  ViewportState zoomedViewportState;
  zoomedViewportState.set(zoomedViewport);

  const auto zoomedPlan = planner.build(
    coverageIndex,
    selectionPolicy,
    zoomedViewportExtent,
    zoomedViewport.scale_denominator);
  REQUIRE_FALSE(zoomedPlan.empty());
  requireProjectedQuiltPlan(zoomedPlan);

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
