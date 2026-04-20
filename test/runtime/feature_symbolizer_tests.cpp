#include <catch2/catch_test_macros.hpp>

#include "chart_data/feature.hpp"
#include "portrayal/feature_symbolizer.hpp"
#include "senc/senc_reader.hpp"
#include "senc/senc_writer.hpp"

#include <algorithm>
#include <array>

namespace {
using chart_view::runtime::chart_data::AreaGeometry;
using chart_view::runtime::chart_data::Feature;
using chart_view::runtime::chart_data::LineGeometry;
using chart_view::runtime::chart_data::PointGeometry;
using chart_view::runtime::portrayal::FeatureSymbolizer;
using chart_view::runtime::portrayal::S52RuleSelectionFilter;
using chart_view::runtime::portrayal::S52DisplaySettings;
using chart_view::runtime::portrayal::S52PointSymbolMode;
using chart_view::runtime::portrayal::S57ClassSelectionFilter;
using chart_view::runtime::portrayal::instructionAssetId;
using chart_view::runtime::portrayal::instructionTextAttributeKey;
using chart_view::runtime::portrayal::instructionType;
using chart_view::runtime::portrayal::S52InstructionType;

std::string utf8Harbor()
{
  return std::string("\xE6\xB8\xAF");
}

std::string utf8HarborA()
{
  return utf8Harbor() + "A";
}

}

TEST_CASE("FeatureSymbolizer applies S57 rule-table mappings for key classes", "[portrayal][symbolizer][s57]")
{
  FeatureSymbolizer symbolizer;

  Feature wreck;
  wreck.classAcronym = "WRECKS";
  wreck.geometry = PointGeometry{{121.0, 31.0}};

  Feature fairway;
  fairway.classAcronym = "FAIRWY";
  fairway.geometry = LineGeometry{{{121.0, 31.0}, {121.2, 31.2}}};

  Feature landArea;
  landArea.classAcronym = "LNDARE";
  landArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.2, 31.0}, {121.2, 31.2}}, {}};

  Feature restrictedArea;
  restrictedArea.classAcronym = "RESARE";
  restrictedArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.3, 31.0}, {121.3, 31.3}}, {}};

  const auto wreckStyle = symbolizer.symbolize(wreck);
  const auto fairwayStyle = symbolizer.symbolize(fairway);
  const auto landStyle = symbolizer.symbolize(landArea);
  const auto restrictedStyle = symbolizer.symbolize(restrictedArea);

  REQUIRE(wreckStyle.styleKey == "point/danger");
  if(wreckStyle.s52Lookup.has_value()) {
    REQUIRE_FALSE(wreckStyle.s52Lookup->ruleId.empty());
  }

  REQUIRE(fairwayStyle.styleKey == "line/channel");
  if(fairwayStyle.s52Lookup.has_value()) {
    REQUIRE_FALSE(fairwayStyle.s52Lookup->ruleId.empty());
  }

  REQUIRE(landStyle.styleKey == "area/land");
  if(landStyle.s52Lookup.has_value()) {
    REQUIRE_FALSE(landStyle.s52Lookup->ruleId.empty());
  }

  REQUIRE(restrictedStyle.styleKey == "area/restricted");
  if(restrictedStyle.s52Lookup.has_value()) {
    REQUIRE_FALSE(restrictedStyle.s52Lookup->ruleId.empty());
  }
}

TEST_CASE("FeatureSymbolizer applies S101 rule-table mappings for key classes", "[portrayal][symbolizer][s101]")
{
  FeatureSymbolizer symbolizer;

  Feature wreck;
  wreck.classAcronym = "Wreck";
  wreck.geometry = PointGeometry{{121.0, 31.0}};

  Feature fairway;
  fairway.classAcronym = "Fairway";
  fairway.geometry = LineGeometry{{{121.0, 31.0}, {121.2, 31.2}}};

  Feature landArea;
  landArea.classAcronym = "LandArea";
  landArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.2, 31.0}, {121.2, 31.2}}, {}};

  Feature restrictedArea;
  restrictedArea.classAcronym = "RestrictedArea";
  restrictedArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.3, 31.0}, {121.3, 31.3}}, {}};

  REQUIRE(symbolizer.symbolize(wreck).styleKey == "point/danger");
  REQUIRE_FALSE(symbolizer.symbolize(wreck).s52Lookup.has_value());
  REQUIRE(symbolizer.symbolize(fairway).styleKey == "line/channel");
  REQUIRE_FALSE(symbolizer.symbolize(fairway).s52Lookup.has_value());
  REQUIRE(symbolizer.symbolize(landArea).styleKey == "area/land");
  REQUIRE_FALSE(symbolizer.symbolize(landArea).s52Lookup.has_value());
  REQUIRE(symbolizer.symbolize(restrictedArea).styleKey == "area/restricted");
  REQUIRE_FALSE(symbolizer.symbolize(restrictedArea).s52Lookup.has_value());
}

TEST_CASE("FeatureSymbolizer maps point features to point style keys", "[portrayal][symbolizer]")
{
  FeatureSymbolizer symbolizer;

  Feature sounding;
  sounding.classAcronym = "SOUNDG";
  sounding.geometry = PointGeometry{{121.0, 31.0}};
  sounding.attributes["VALSOU"] = 12.5;

  Feature buoy;
  buoy.classAcronym = "BOYSPP";
  buoy.geometry = PointGeometry{{121.1, 31.1}};

  Feature beacon;
  beacon.classAcronym = "BCNSPP";
  beacon.geometry = PointGeometry{{121.2, 31.2}};

  Feature genericPoint;
  genericPoint.classAcronym = "OBSTRN";
  genericPoint.geometry = PointGeometry{{121.3, 31.3}};

  const auto soundingStyle = symbolizer.symbolize(sounding);
  const auto buoyStyle = symbolizer.symbolize(buoy);
  const auto beaconStyle = symbolizer.symbolize(beacon);
  const auto genericStyle = symbolizer.symbolize(genericPoint);

  REQUIRE(soundingStyle.geometryType == chart_view::runtime::chart_data::GeometryType::kPoint);
  REQUIRE(soundingStyle.styleKey == "point/sounding");
  REQUIRE(soundingStyle.s52Lookup.has_value());
  REQUIRE_FALSE(soundingStyle.s52Lookup->ruleId.empty());
  REQUIRE(buoyStyle.styleKey == "point/buoy");
  REQUIRE(buoyStyle.s52Lookup.has_value());
  REQUIRE_FALSE(buoyStyle.s52Lookup->ruleId.empty());
  REQUIRE(beaconStyle.styleKey == "point/beacon");
  REQUIRE(beaconStyle.s52Lookup.has_value());
  REQUIRE_FALSE(beaconStyle.s52Lookup->ruleId.empty());
  REQUIRE(genericStyle.styleKey == "point/default");
  REQUIRE(genericStyle.s52Lookup.has_value());
  REQUIRE_FALSE(genericStyle.s52Lookup->ruleId.empty());
}

TEST_CASE("FeatureSymbolizer maps line features to line style keys", "[portrayal][symbolizer]")
{
  FeatureSymbolizer symbolizer;

  Feature depthContour;
  depthContour.classAcronym = "DEPCNT";
  depthContour.geometry = LineGeometry{{{121.0, 31.0}, {121.2, 31.2}}};
  depthContour.attributes["VALDCO"] = 20.0;

  Feature coastline;
  coastline.classAcronym = "COALNE";
  coastline.geometry = LineGeometry{{{121.0, 31.0}, {121.3, 31.2}}};
  coastline.attributes["CATCOA"] = std::int64_t{1};

  Feature defaultLine;
  defaultLine.classAcronym = "BRIDGE";
  defaultLine.geometry = LineGeometry{{{121.0, 31.0}, {121.4, 31.2}}};

  const auto depthContourStyle = symbolizer.symbolize(depthContour);
  const auto coastlineStyle = symbolizer.symbolize(coastline);
  const auto defaultLineStyle = symbolizer.symbolize(defaultLine);

  REQUIRE(depthContourStyle.styleKey == "line/depth_contour");
  REQUIRE(depthContourStyle.s52Lookup.has_value());
  REQUIRE_FALSE(depthContourStyle.s52Lookup->ruleId.empty());
  REQUIRE(coastlineStyle.styleKey == "line/coastline");
  REQUIRE(coastlineStyle.s52Lookup.has_value());
  REQUIRE_FALSE(coastlineStyle.s52Lookup->ruleId.empty());
  REQUIRE(defaultLineStyle.styleKey == "line/default");
  REQUIRE(defaultLineStyle.s52Lookup.has_value());
  REQUIRE_FALSE(defaultLineStyle.s52Lookup->ruleId.empty());
}

TEST_CASE("FeatureSymbolizer maps area features and text attributes", "[portrayal][symbolizer]")
{
  FeatureSymbolizer symbolizer;

  Feature depthArea;
  depthArea.classAcronym = "DEPARE";
  depthArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.2, 31.0}, {121.2, 31.2}}, {}};
  depthArea.attributes["DRVAL1"] = 5.0;
  depthArea.attributes["OBJNAM"] = std::string("Depth Area");

  Feature genericArea;
  genericArea.classAcronym = "ACHARE";
  genericArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.3, 31.0}, {121.3, 31.3}}, {}};

  const auto depthAreaStyle = symbolizer.symbolize(depthArea);
  const auto genericAreaStyle = symbolizer.symbolize(genericArea);

  REQUIRE(depthAreaStyle.geometryType == chart_view::runtime::chart_data::GeometryType::kArea);
  REQUIRE(depthAreaStyle.styleKey == "area/depth");
  REQUIRE(depthAreaStyle.textKey == "text/default");
  REQUIRE(depthAreaStyle.textAttributeKey == "OBJNAM");
  REQUIRE(depthAreaStyle.s52Lookup.has_value());
  REQUIRE(depthAreaStyle.s52Lookup->instructions.size() >= 2);
  REQUIRE_FALSE(depthAreaStyle.s52Lookup->ruleId.empty());
  const auto conditionalCount = std::count_if(
    depthAreaStyle.s52Lookup->instructions.begin(),
    depthAreaStyle.s52Lookup->instructions.end(),
    [](const auto &instruction) { return instructionType(instruction) == S52InstructionType::kConditional; });
  REQUIRE(conditionalCount >= 2);
  const auto textInstructionCount = std::count_if(
    depthAreaStyle.s52Lookup->instructions.begin(),
    depthAreaStyle.s52Lookup->instructions.end(),
    [](const auto &instruction) { return instructionTextAttributeKey(instruction) == "OBJNAM"; });
  REQUIRE(textInstructionCount == 1);
  REQUIRE(genericAreaStyle.styleKey == "area/default");
  REQUIRE(genericAreaStyle.s52Lookup.has_value());
  REQUIRE_FALSE(genericAreaStyle.s52Lookup->ruleId.empty());
  REQUIRE(genericAreaStyle.textKey == "text/default");
  REQUIRE(genericAreaStyle.textAttributeKey == "OBJNAM");
}

TEST_CASE("FeatureSymbolizer exposes official e4.0.0 wave-1 assets on the mainline",
          "[portrayal][symbolizer][official][wave1]")
{
  FeatureSymbolizer symbolizer;

  Feature floatingHazard;
  floatingHazard.classAcronym = "OBSTRN";
  floatingHazard.geometry = PointGeometry{{121.0, 31.0}};
  floatingHazard.attributes["CATOBS"] = std::int64_t{8};
  floatingHazard.attributes["VALSOU"] = 0.5;

  Feature essaArea;
  essaArea.classAcronym = "RESARE";
  essaArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.2, 31.0}, {121.2, 31.2}}, {}};
  essaArea.attributes["CATREA"] = std::int64_t{27};

  Feature pssaArea;
  pssaArea.classAcronym = "RESARE";
  pssaArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.3, 31.0}, {121.3, 31.3}}, {}};
  pssaArea.attributes["CATREA"] = std::int64_t{28};

  const auto floatingHazardStyle = symbolizer.symbolize(floatingHazard);
  const auto essaStyle = symbolizer.symbolize(essaArea);
  const auto pssaStyle = symbolizer.symbolize(pssaArea);

  REQUIRE(floatingHazardStyle.s52Lookup.has_value());
  REQUIRE_FALSE(floatingHazardStyle.s52Lookup->instructionFallback);
  REQUIRE(std::any_of(
    floatingHazardStyle.s52Lookup->instructions.begin(),
    floatingHazardStyle.s52Lookup->instructions.end(),
    [](const auto &instruction) {
      return instructionType(instruction) == S52InstructionType::kPointSymbol
          && instructionAssetId(instruction) == "FLTHAZ02";
    }));

  REQUIRE(essaStyle.s52Lookup.has_value());
  REQUIRE_FALSE(essaStyle.s52Lookup->instructionFallback);
  REQUIRE(std::any_of(
    essaStyle.s52Lookup->instructions.begin(),
    essaStyle.s52Lookup->instructions.end(),
    [](const auto &instruction) {
      return instructionType(instruction) == S52InstructionType::kPointSymbol
          && instructionAssetId(instruction) == "ESSARE01";
    }));
  REQUIRE(std::any_of(
    essaStyle.s52Lookup->instructions.begin(),
    essaStyle.s52Lookup->instructions.end(),
    [](const auto &instruction) {
      return instructionType(instruction) == S52InstructionType::kLineStyle
          && instructionAssetId(instruction) == "ESSARE01";
    }));

  REQUIRE(pssaStyle.s52Lookup.has_value());
  REQUIRE_FALSE(pssaStyle.s52Lookup->instructionFallback);
  REQUIRE(std::any_of(
    pssaStyle.s52Lookup->instructions.begin(),
    pssaStyle.s52Lookup->instructions.end(),
    [](const auto &instruction) {
      return instructionType(instruction) == S52InstructionType::kPointSymbol
          && instructionAssetId(instruction) == "PSSARE01";
    }));
  REQUIRE(std::any_of(
    pssaStyle.s52Lookup->instructions.begin(),
    pssaStyle.s52Lookup->instructions.end(),
    [](const auto &instruction) {
      return instructionType(instruction) == S52InstructionType::kLineStyle
          && instructionAssetId(instruction) == "ESSARE01";
    }));
}

TEST_CASE("FeatureSymbolizer preserves compiled text attribute choices even when source text is absent",
          "[portrayal][symbolizer][text]")
{
  FeatureSymbolizer symbolizer;

  Feature landArea;
  landArea.classAcronym = "LNDARE";
  landArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.2, 31.0}, {121.2, 31.2}}, {}};

  Feature buoy;
  buoy.classAcronym = "BOYSPP";
  buoy.geometry = PointGeometry{{121.1, 31.1}};

  const auto landStyle = symbolizer.symbolize(landArea);
  REQUIRE(landStyle.s52Lookup.has_value());
  REQUIRE(landStyle.textKey == "text/default");
  REQUIRE(landStyle.textAttributeKey == "OBJNAM");

  const auto buoyStyle = symbolizer.symbolize(buoy);
  REQUIRE(buoyStyle.s52Lookup.has_value());
  REQUIRE(buoyStyle.textKey == "text/default");
  REQUIRE(buoyStyle.textAttributeKey == "OBJNAM");
}

TEST_CASE("FeatureSymbolizer handles SENC-roundtripped feature attributes", "[portrayal][symbolizer][senc]")
{
  using chart_view::runtime::chart_data::DatasetMeta;
  using chart_view::runtime::chart_data::FeatureChartDataset;
  using chart_view::runtime::senc::SencReader;
  using chart_view::runtime::senc::SencWriter;

  FeatureChartDataset dataset;
  DatasetMeta meta;
  meta.name = "symbolizer_roundtrip";
  meta.sourceType = chart_view_chart_source_s57;
  meta.extent = {121.0, 31.0, 121.3, 31.3};
  dataset.setMeta(std::move(meta));

  Feature sounding;
  sounding.id = 1;
  sounding.classAcronym = "SOUNDG";
  sounding.geometry = PointGeometry{{121.0, 31.0}};
  sounding.attributes["VALSOU"] = 12.5;
  dataset.addFeature(std::move(sounding));

  Feature coastline;
  coastline.id = 2;
  coastline.classAcronym = "COALNE";
  coastline.geometry = LineGeometry{{{121.0, 31.0}, {121.2, 31.2}}};
  coastline.attributes["CATCOA"] = std::int64_t{1};
  dataset.addFeature(std::move(coastline));

  Feature depthArea;
  depthArea.id = 3;
  depthArea.classAcronym = "DEPARE";
  depthArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.3, 31.0}, {121.3, 31.3}}, {}};
  depthArea.attributes["DRVAL1"] = 5.0;
  depthArea.attributes["OBJNAM"] = std::string("Depth Area");
  depthArea.attributes["NOBJNM"] = utf8HarborA();
  dataset.addFeature(std::move(depthArea));

  SencWriter writer;
  const auto blob = writer.write(dataset);
  REQUIRE_FALSE(blob.empty());

  SencReader reader;
  const auto readback = reader.read(blob);
  REQUIRE(readback.ok);
  REQUIRE(readback.dataset.featureCount() == 3);

  FeatureSymbolizer symbolizer;
  const auto pointStyle = symbolizer.symbolize(readback.dataset.features()[0]);
  REQUIRE(pointStyle.styleKey == "point/sounding");
  REQUIRE(pointStyle.s52Lookup.has_value());
  const auto lineStyle = symbolizer.symbolize(readback.dataset.features()[1]);
  REQUIRE(lineStyle.styleKey == "line/coastline");
  REQUIRE(lineStyle.s52Lookup.has_value());
  const auto areaStyle = symbolizer.symbolize(readback.dataset.features()[2]);
  REQUIRE(areaStyle.styleKey == "area/depth");
  REQUIRE(areaStyle.textKey == "text/default");
  REQUIRE(areaStyle.textAttributeKey == "NOBJNM");
  REQUIRE(areaStyle.s52Lookup.has_value());
}

TEST_CASE("FeatureSymbolizer applies S52 display settings and conditional symbology", "[portrayal][symbolizer][s52][settings]")
{
  S52DisplaySettings settings;
  settings.pointSymbolMode = S52PointSymbolMode::kSimplified;
  settings.showSoundings = false;
  settings.showTextLabels = false;

  FeatureSymbolizer symbolizer(settings);

  Feature sounding;
  sounding.classAcronym = "SOUNDG";
  sounding.geometry = PointGeometry{{121.0, 31.0}};
  sounding.attributes["VALSOU"] = 12.5;
  sounding.attributes["OBJNAM"] = std::string("Hidden sounding");

  Feature buoy;
  buoy.classAcronym = "BOYSPP";
  buoy.geometry = PointGeometry{{121.1, 31.1}};

  Feature namedWreck;
  namedWreck.classAcronym = "WRECKS";
  namedWreck.geometry = PointGeometry{{121.2, 31.2}};
  namedWreck.attributes["OBJNAM"] = std::string("Named Wreck");

  const auto soundingStyle = symbolizer.symbolize(sounding);
  REQUIRE(soundingStyle.s52Lookup.has_value());
  REQUIRE(soundingStyle.suppressed);
  REQUIRE(soundingStyle.styleKey.empty());
  REQUIRE(soundingStyle.textKey.empty());
  REQUIRE(soundingStyle.textAttributeKey.empty());

  const auto buoyStyle = symbolizer.symbolize(buoy);
  REQUIRE(buoyStyle.s52Lookup.has_value());
  REQUIRE_FALSE(buoyStyle.suppressed);
  REQUIRE(instructionAssetId(buoyStyle.s52Lookup->instructions.front()) == "BOYSPP11");
  REQUIRE(buoyStyle.textKey.empty());
  REQUIRE(buoyStyle.textAttributeKey == "OBJNAM");

  const auto wreckStyle = symbolizer.symbolize(namedWreck);
  REQUIRE(wreckStyle.s52Lookup.has_value());
  REQUIRE_FALSE(wreckStyle.suppressed);
  REQUIRE(wreckStyle.s52Lookup->instructions.size() == 1);
  REQUIRE(wreckStyle.textKey.empty());
  REQUIRE(wreckStyle.textAttributeKey.empty());
}

TEST_CASE("FeatureSymbolizer honors S57 class and stable rule selection filters", "[portrayal][symbolizer][filters]")
{
  FeatureSymbolizer baselineSymbolizer;

  Feature wreck;
  wreck.classAcronym = "WRECKS";
  wreck.geometry = PointGeometry{{121.0, 31.0}};

  Feature depthArea;
  depthArea.classAcronym = "DEPARE";
  depthArea.geometry = AreaGeometry{{{121.0, 31.0}, {121.2, 31.0}, {121.2, 31.2}}, {}};
  depthArea.attributes["DRVAL1"] = 5.0;

  Feature buoy;
  buoy.classAcronym = "BOYSPP";
  buoy.geometry = PointGeometry{{121.1, 31.1}};

  const auto baselineWreckStyle = baselineSymbolizer.symbolize(wreck);
  const auto baselineDepthAreaStyle = baselineSymbolizer.symbolize(depthArea);
  REQUIRE(baselineWreckStyle.s52Lookup.has_value());
  REQUIRE(baselineDepthAreaStyle.s52Lookup.has_value());

  FeatureSymbolizer symbolizer;
  const std::array classFilters{
    S57ClassSelectionFilter{"DEPARE", false}};
  const std::array ruleFilters{
    S52RuleSelectionFilter{baselineWreckStyle.s52Lookup->ruleId, false},
    S52RuleSelectionFilter{baselineDepthAreaStyle.s52Lookup->ruleId, true}};
  symbolizer.setS57ClassFilters(classFilters);
  symbolizer.setS52RuleFilters(ruleFilters);

  const auto wreckStyle = symbolizer.symbolize(wreck);
  REQUIRE(wreckStyle.s52Lookup.has_value());
  REQUIRE(wreckStyle.s52Lookup->ruleId == baselineWreckStyle.s52Lookup->ruleId);
  REQUIRE(wreckStyle.suppressed);
  REQUIRE(wreckStyle.styleKey.empty());

  const auto depthAreaStyle = symbolizer.symbolize(depthArea);
  REQUIRE(depthAreaStyle.s52Lookup.has_value());
  REQUIRE(depthAreaStyle.s52Lookup->ruleId == baselineDepthAreaStyle.s52Lookup->ruleId);
  REQUIRE(depthAreaStyle.suppressed);
  REQUIRE(depthAreaStyle.styleKey.empty());

  const auto buoyStyle = symbolizer.symbolize(buoy);
  REQUIRE(buoyStyle.s52Lookup.has_value());
  REQUIRE_FALSE(buoyStyle.suppressed);
}
