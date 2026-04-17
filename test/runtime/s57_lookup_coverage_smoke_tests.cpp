#include <catch2/catch_test_macros.hpp>

#include "portrayal/feature_symbolizer.hpp"
#include "portrayal/s52_display_settings.hpp"
#include "portrayal/s52_instruction_ir.hpp"
#include "portrayal/s52_lookup_model.hpp"
#include "s57/s57_reader.hpp"
#include "senc/senc_reader.hpp"
#include "senc/senc_writer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

using chart_view::runtime::chart_data::Extent;
using chart_view::runtime::chart_data::FeatureChartDataset;
using chart_view::runtime::portrayal::FeatureSymbolizer;
using chart_view::runtime::portrayal::S52DisplaySettings;
using chart_view::runtime::portrayal::S52InstructionType;
using chart_view::runtime::portrayal::S52LookupModel;
using chart_view::runtime::portrayal::instructionType;

struct LoadedChart
{
  std::filesystem::path sourcePath;
  chart_view::runtime::s57::S57ReadResult readResult;
  FeatureChartDataset preparedDataset;
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

std::vector<LoadedChart> loadReadableCharts(const std::filesystem::path &root)
{
  chart_view::runtime::s57::S57Reader reader;
  std::vector<LoadedChart> loadedCharts;

  for(const auto &chartPath : findS57Charts(root)) {
    auto readResult = reader.read(chartPath.string());
    if(!readResult.ok || readResult.dataset.empty() || !readResult.dataset.meta().extent.isValid()) {
      continue;
    }

    auto preparedDataset = prepareDatasetForSmoke(chartPath, readResult.dataset);
    loadedCharts.push_back({
      chartPath,
      std::move(readResult),
      std::move(preparedDataset)});
  }

  return loadedCharts;
}

std::size_t countS52Hits(
  const FeatureChartDataset &dataset,
  const FeatureSymbolizer &symbolizer)
{
  std::size_t count = 0;
  for(const auto &feature : dataset.features()) {
    if(symbolizer.symbolize(feature).s52Lookup.has_value()) {
      ++count;
    }
  }
  return count;
}

std::size_t countPreferredCompiledHits(const FeatureChartDataset &dataset)
{
  std::size_t count = 0;
  for(const auto &feature : dataset.features()) {
    const auto lookup = S52LookupModel::lookup(feature);
    if(lookup.has_value() && !lookup->instructionFallback) {
      ++count;
    }
  }
  return count;
}

std::size_t countFallbackCompiledHits(const FeatureChartDataset &dataset)
{
  std::size_t count = 0;
  for(const auto &feature : dataset.features()) {
    const auto lookup = S52LookupModel::lookup(feature);
    if(lookup.has_value() && lookup->instructionFallback) {
      ++count;
    }
  }
  return count;
}

std::size_t countPreferredTextInstructionHits(const FeatureChartDataset &dataset)
{
  std::size_t count = 0;
  for(const auto &feature : dataset.features()) {
    const auto lookup = S52LookupModel::lookup(feature);
    if(!lookup.has_value() || lookup->instructionFallback) {
      continue;
    }

    const auto hasTextInstruction = std::any_of(
      lookup->instructions.begin(),
      lookup->instructions.end(),
      [](const auto &instruction) {
        return instructionType(instruction) == S52InstructionType::kTextLabel;
      });
    if(hasTextInstruction) {
      ++count;
    }
  }
  return count;
}

enum class CoverageFamily
{
  kAidToNavigation,
  kHazardPoints,
  kLineAndBoundary,
  kAreaPatterns,
  kNamedText,
};

constexpr std::array<CoverageFamily, 5> kCoverageFamilies{
  CoverageFamily::kAidToNavigation,
  CoverageFamily::kHazardPoints,
  CoverageFamily::kLineAndBoundary,
  CoverageFamily::kAreaPatterns,
  CoverageFamily::kNamedText,
};

struct CoverageFamilyMetrics
{
  std::size_t featuresSeen{0};
  std::size_t s52Hits{0};
  std::size_t preferredCompiledHits{0};
  std::size_t fallbackHits{0};
  std::size_t preferredTextInstructionHits{0};
};

enum class Wave1CoverageFamily
{
  kNoticesAndTerminals,
  kLateralAndWaterwayMarks,
  kHarbourFacilitiesAndPositions,
  kHazardsAndServices,
};

constexpr std::array<Wave1CoverageFamily, 4> kWave1CoverageFamilies{
  Wave1CoverageFamily::kNoticesAndTerminals,
  Wave1CoverageFamily::kLateralAndWaterwayMarks,
  Wave1CoverageFamily::kHarbourFacilitiesAndPositions,
  Wave1CoverageFamily::kHazardsAndServices,
};

bool hasNonEmptyTextAttribute(const chart_view::runtime::chart_data::Feature &feature, std::string_view key)
{
  const auto it = feature.attributes.find(std::string(key));
  if(it == feature.attributes.end()) {
    return false;
  }

  if(const auto *value = std::get_if<std::string>(&it->second)) {
    return !value->empty();
  }

  if(const auto *values = std::get_if<chart_view::runtime::chart_data::AttributeStringList>(&it->second)) {
    return std::any_of(values->begin(), values->end(), [](const auto &entry) { return !entry.empty(); });
  }

  return false;
}

std::string_view familyName(CoverageFamily family) noexcept
{
  switch(family) {
  case CoverageFamily::kAidToNavigation:
    return "aid_to_navigation";
  case CoverageFamily::kHazardPoints:
    return "hazard_points";
  case CoverageFamily::kLineAndBoundary:
    return "line_and_boundary";
  case CoverageFamily::kAreaPatterns:
    return "area_patterns";
  case CoverageFamily::kNamedText:
    return "named_text";
  }

  return "unknown";
}

std::vector<CoverageFamily> familiesForFeature(const chart_view::runtime::chart_data::Feature &feature)
{
  std::vector<CoverageFamily> result;
  const auto &acronym = feature.classAcronym;

  if(acronym == "BOYSPP" || acronym == "BCNSPP" || acronym == "TOPMAR" || acronym == "LIGHTS") {
    result.push_back(CoverageFamily::kAidToNavigation);
  }

  if(acronym == "WRECKS" || acronym == "OBSTRN" || acronym == "UWTROC") {
    result.push_back(CoverageFamily::kHazardPoints);
  }

  if(acronym == "COALNE" || acronym == "DEPCNT" || acronym == "FAIRWY" || acronym == "PIPSOL"
     || acronym == "CBLARE" || acronym == "CBLOHD") {
    result.push_back(CoverageFamily::kLineAndBoundary);
  }

  if(acronym == "LNDARE" || acronym == "DEPARE" || acronym == "ACHARE" || acronym == "RESARE"
     || acronym == "SEAARE" || acronym == "RIVERS" || acronym == "CANALS") {
    result.push_back(CoverageFamily::kAreaPatterns);
  }

  if(hasNonEmptyTextAttribute(feature, "OBJNAM") || hasNonEmptyTextAttribute(feature, "NOBJNM")) {
    result.push_back(CoverageFamily::kNamedText);
  }

  return result;
}

using CoverageFamilyMetricMap = std::unordered_map<CoverageFamily, CoverageFamilyMetrics>;
using Wave1CoverageFamilyMetricMap = std::unordered_map<Wave1CoverageFamily, CoverageFamilyMetrics>;

CoverageFamilyMetricMap initializeCoverageMetrics()
{
  CoverageFamilyMetricMap metrics;
  for(const auto family : kCoverageFamilies) {
    metrics.emplace(family, CoverageFamilyMetrics{});
  }
  return metrics;
}

Wave1CoverageFamilyMetricMap initializeWave1CoverageMetrics()
{
  Wave1CoverageFamilyMetricMap metrics;
  for(const auto family : kWave1CoverageFamilies) {
    metrics.emplace(family, CoverageFamilyMetrics{});
  }
  return metrics;
}

void accumulateCoverageMetrics(
  CoverageFamilyMetricMap &metrics,
  const chart_view::runtime::chart_data::Feature &feature)
{
  const auto families = familiesForFeature(feature);
  if(families.empty()) {
    return;
  }

  const auto lookup = S52LookupModel::lookup(feature);
  const auto hasPreferredTextInstruction = lookup.has_value() && !lookup->instructionFallback
                                        && std::any_of(
                                          lookup->instructions.begin(),
                                          lookup->instructions.end(),
                                          [](const auto &instruction) {
                                            return instructionType(instruction)
                                                == S52InstructionType::kTextLabel;
                                          });

  for(const auto family : families) {
    auto &familyMetrics = metrics[family];
    ++familyMetrics.featuresSeen;
    if(!lookup.has_value()) {
      continue;
    }

    ++familyMetrics.s52Hits;
    if(lookup->instructionFallback) {
      ++familyMetrics.fallbackHits;
    } else {
      ++familyMetrics.preferredCompiledHits;
      if(hasPreferredTextInstruction) {
        ++familyMetrics.preferredTextInstructionHits;
      }
    }
  }
}

void printCoverageMetrics(std::string_view prefix, const CoverageFamilyMetricMap &metrics)
{
  std::cout << prefix;
  for(const auto family : kCoverageFamilies) {
    const auto it = metrics.find(family);
    if(it == metrics.end()) {
      continue;
    }

    const auto &familyMetrics = it->second;
    std::cout << ' ' << familyName(family)
              << "{seen=" << familyMetrics.featuresSeen
              << ",s52Hits=" << familyMetrics.s52Hits
              << ",preferredCompiledHits=" << familyMetrics.preferredCompiledHits
              << ",fallbackHits=" << familyMetrics.fallbackHits
              << ",preferredTextInstructionHits=" << familyMetrics.preferredTextInstructionHits
              << '}';
  }
  std::cout << std::endl;
}

std::string_view wave1FamilyName(Wave1CoverageFamily family) noexcept
{
  switch(family) {
  case Wave1CoverageFamily::kNoticesAndTerminals:
    return "notices_and_terminals";
  case Wave1CoverageFamily::kLateralAndWaterwayMarks:
    return "lateral_and_waterway_marks";
  case Wave1CoverageFamily::kHarbourFacilitiesAndPositions:
    return "harbour_facilities_and_positions";
  case Wave1CoverageFamily::kHazardsAndServices:
    return "hazards_and_services";
  }

  return "unknown";
}

std::vector<Wave1CoverageFamily> wave1FamiliesForFeature(const chart_view::runtime::chart_data::Feature &feature)
{
  std::vector<Wave1CoverageFamily> result;
  const auto &acronym = feature.classAcronym;

  if(acronym == "NOTMRK" || acronym == "TERMNL") {
    result.push_back(Wave1CoverageFamily::kNoticesAndTerminals);
  }

  if(acronym == "BOYLAT" || acronym == "BOYWTW" || acronym == "BCNLAT" || acronym == "TOPMAR") {
    result.push_back(Wave1CoverageFamily::kLateralAndWaterwayMarks);
  }

  if(acronym == "HRBFAC" || acronym == "POSITN") {
    result.push_back(Wave1CoverageFamily::kHarbourFacilitiesAndPositions);
  }

  if(acronym == "OBSTRN" || acronym == "RDOCAL" || acronym == "VEHTRF" || acronym == "RESARE") {
    result.push_back(Wave1CoverageFamily::kHazardsAndServices);
  }

  return result;
}

void accumulateWave1CoverageMetrics(
  Wave1CoverageFamilyMetricMap &metrics,
  const chart_view::runtime::chart_data::Feature &feature)
{
  const auto families = wave1FamiliesForFeature(feature);
  if(families.empty()) {
    return;
  }

  const auto lookup = S52LookupModel::lookup(feature);
  const auto hasPreferredTextInstruction = lookup.has_value() && !lookup->instructionFallback
                                        && std::any_of(
                                          lookup->instructions.begin(),
                                          lookup->instructions.end(),
                                          [](const auto &instruction) {
                                            return instructionType(instruction)
                                                == S52InstructionType::kTextLabel;
                                          });

  for(const auto family : families) {
    auto &familyMetrics = metrics[family];
    ++familyMetrics.featuresSeen;
    if(!lookup.has_value()) {
      continue;
    }

    ++familyMetrics.s52Hits;
    if(lookup->instructionFallback) {
      ++familyMetrics.fallbackHits;
    } else {
      ++familyMetrics.preferredCompiledHits;
      if(hasPreferredTextInstruction) {
        ++familyMetrics.preferredTextInstructionHits;
      }
    }
  }
}

void printWave1CoverageMetrics(std::string_view prefix, const Wave1CoverageFamilyMetricMap &metrics)
{
  std::cout << prefix;
  for(const auto family : kWave1CoverageFamilies) {
    const auto it = metrics.find(family);
    if(it == metrics.end()) {
      continue;
    }

    const auto &familyMetrics = it->second;
    std::cout << ' ' << wave1FamilyName(family)
              << "{seen=" << familyMetrics.featuresSeen
              << ",s52Hits=" << familyMetrics.s52Hits
              << ",preferredCompiledHits=" << familyMetrics.preferredCompiledHits
              << ",fallbackHits=" << familyMetrics.fallbackHits
              << ",preferredTextInstructionHits=" << familyMetrics.preferredTextInstructionHits
              << '}';
  }
  std::cout << std::endl;
}

} // namespace

TEST_CASE("S57 lookup coverage smoke validates preferred compiled OpenCPN-derived rule hits on real charts",
          "[s57][phase6a][lookup][real][smoke]")
{
  const auto root = getS57Root();
  auto loadedCharts = loadReadableCharts(root);
  if(loadedCharts.size() < 3U) {
    SKIP("Need at least three readable S57 charts for the Phase 6A lookup-coverage smoke");
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
    SKIP("Known fixed-pair charts C1511781/C1511782 not available for Phase 6A lookup-coverage smoke");
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
      break;
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
    SKIP("Need fixed pair plus one additional readable S57 chart for Phase 6A lookup-coverage smoke");
  }

  S52DisplaySettings settings;
  settings.pointSymbolMode = chart_view::runtime::portrayal::S52PointSymbolMode::kSimplified;
  FeatureSymbolizer symbolizer(settings);

  std::size_t totalS52Hits = 0;
  std::size_t totalPreferredHits = 0;
  std::size_t totalFallbackHits = 0;
  std::size_t totalPreferredTextHits = 0;
  auto totalFamilyMetrics = initializeCoverageMetrics();
  auto totalWave1FamilyMetrics = initializeWave1CoverageMetrics();

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

    const auto s52Hits = countS52Hits(readback.dataset, symbolizer);
    const auto preferredHits = countPreferredCompiledHits(readback.dataset);
    const auto fallbackHits = countFallbackCompiledHits(readback.dataset);
    const auto preferredTextHits = countPreferredTextInstructionHits(readback.dataset);
    auto chartFamilyMetrics = initializeCoverageMetrics();
    auto chartWave1FamilyMetrics = initializeWave1CoverageMetrics();
    for(const auto &feature : readback.dataset.features()) {
      accumulateCoverageMetrics(chartFamilyMetrics, feature);
      accumulateCoverageMetrics(totalFamilyMetrics, feature);
      accumulateWave1CoverageMetrics(chartWave1FamilyMetrics, feature);
      accumulateWave1CoverageMetrics(totalWave1FamilyMetrics, feature);
    }

    std::cout << "lookup-coverage sample: " << readback.dataset.meta().name
              << " usageBand=" << readback.dataset.meta().usageBand
              << " features=" << readback.dataset.featureCount()
              << " s52Hits=" << s52Hits
              << " preferredCompiledHits=" << preferredHits
              << " fallbackCompiledHits=" << fallbackHits
              << " preferredTextInstructionHits=" << preferredTextHits
              << std::endl;
    printCoverageMetrics("lookup-coverage families:", chartFamilyMetrics);
    printWave1CoverageMetrics("phase6c wave1 families:", chartWave1FamilyMetrics);

    REQUIRE(s52Hits > 0U);
    REQUIRE(preferredHits > 0U);
    totalS52Hits += s52Hits;
    totalPreferredHits += preferredHits;
    totalFallbackHits += fallbackHits;
    totalPreferredTextHits += preferredTextHits;
  }

  std::cout << "phase6a lookup coverage totals:"
            << " totalS52Hits=" << totalS52Hits
            << " totalPreferredCompiledHits=" << totalPreferredHits
            << " totalFallbackCompiledHits=" << totalFallbackHits
            << " totalPreferredTextInstructionHits=" << totalPreferredTextHits
            << std::endl;
  printCoverageMetrics("phase6b lookup coverage family totals:", totalFamilyMetrics);
  printWave1CoverageMetrics("phase6c wave1 family totals:", totalWave1FamilyMetrics);

  REQUIRE(selectedIds.contains(std::string(kTargetChartAStem)));
  REQUIRE(selectedIds.contains(std::string(kTargetChartBStem)));
  REQUIRE(totalS52Hits > 0U);
  REQUIRE(totalPreferredHits > 0U);
  REQUIRE(totalPreferredTextHits > 0U);
  REQUIRE(totalFamilyMetrics[CoverageFamily::kAidToNavigation].featuresSeen > 0U);
  REQUIRE(totalFamilyMetrics[CoverageFamily::kAidToNavigation].preferredCompiledHits > 0U);
  REQUIRE(totalFamilyMetrics[CoverageFamily::kLineAndBoundary].featuresSeen > 0U);
  REQUIRE(totalFamilyMetrics[CoverageFamily::kLineAndBoundary].preferredCompiledHits > 0U);
  REQUIRE(totalFamilyMetrics[CoverageFamily::kAreaPatterns].featuresSeen > 0U);
  REQUIRE(totalFamilyMetrics[CoverageFamily::kAreaPatterns].preferredCompiledHits > 0U);
  REQUIRE(totalFamilyMetrics[CoverageFamily::kNamedText].featuresSeen > 0U);
  REQUIRE(totalFamilyMetrics[CoverageFamily::kNamedText].preferredTextInstructionHits > 0U);
  if(totalFamilyMetrics[CoverageFamily::kHazardPoints].featuresSeen > 0U) {
    REQUIRE(totalFamilyMetrics[CoverageFamily::kHazardPoints].preferredCompiledHits > 0U);
  }
  for(const auto family : kWave1CoverageFamilies) {
    const auto &metrics = totalWave1FamilyMetrics[family];
    if(metrics.featuresSeen == 0U) {
      INFO("wave1 family has no exposure in retained real-chart sample set: " << wave1FamilyName(family));
      continue;
    }

    INFO("wave1 family=" << wave1FamilyName(family)
                         << " seen=" << metrics.featuresSeen
                         << " preferredCompiledHits=" << metrics.preferredCompiledHits
                         << " fallbackHits=" << metrics.fallbackHits);
    REQUIRE(metrics.preferredCompiledHits > 0U);
    REQUIRE(metrics.fallbackHits == 0U);
  }
}
