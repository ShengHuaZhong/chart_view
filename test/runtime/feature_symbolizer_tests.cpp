#include <catch2/catch_test_macros.hpp>

#include "chart_data/feature.hpp"
#include "portrayal/feature_symbolizer.hpp"
#include "senc/senc_reader.hpp"
#include "senc/senc_writer.hpp"

namespace {
using chart_view::runtime::chart_data::AreaGeometry;
using chart_view::runtime::chart_data::Feature;
using chart_view::runtime::chart_data::LineGeometry;
using chart_view::runtime::chart_data::PointGeometry;
using chart_view::runtime::portrayal::FeatureSymbolizer;
using chart_view::runtime::portrayal::S52DisplaySettings;
using chart_view::runtime::portrayal::S52PointSymbolMode;
using chart_view::runtime::portrayal::instructionAssetId;
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
  REQUIRE(wreckStyle.s52Lookup.has_value());
  REQUIRE(instructionAssetId(wreckStyle.s52Lookup->instructions.front()) == "DANGER01");

  REQUIRE(fairwayStyle.styleKey == "line/channel");
  REQUIRE(fairwayStyle.s52Lookup.has_value());
  REQUIRE(instructionAssetId(fairwayStyle.s52Lookup->instructions.front()) == "FAIRWY01");

  REQUIRE(landStyle.styleKey == "area/land");
  REQUIRE(landStyle.s52Lookup.has_value());
  REQUIRE(instructionAssetId(landStyle.s52Lookup->instructions.front()) == "LNDARE01");

  REQUIRE(restrictedStyle.styleKey == "area/restricted");
  REQUIRE(restrictedStyle.s52Lookup.has_value());
  REQUIRE(instructionAssetId(restrictedStyle.s52Lookup->instructions.front()) == "RESARE01");
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
  REQUIRE(instructionAssetId(soundingStyle.s52Lookup->instructions.front()) == "SOUNDG01");
  REQUIRE(buoyStyle.styleKey == "point/buoy");
  REQUIRE(buoyStyle.s52Lookup.has_value());
  REQUIRE(beaconStyle.styleKey == "point/beacon");
  REQUIRE(beaconStyle.s52Lookup.has_value());
  REQUIRE(genericStyle.styleKey == "point/default");
  REQUIRE_FALSE(genericStyle.s52Lookup.has_value());
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
  REQUIRE(instructionAssetId(depthContourStyle.s52Lookup->instructions.front()) == "DEPCN01");
  REQUIRE(coastlineStyle.styleKey == "line/coastline");
  REQUIRE(coastlineStyle.s52Lookup.has_value());
  REQUIRE(instructionAssetId(coastlineStyle.s52Lookup->instructions.front()) == "COALNE01");
  REQUIRE(defaultLineStyle.styleKey == "line/default");
  REQUIRE_FALSE(defaultLineStyle.s52Lookup.has_value());
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
  REQUIRE(depthAreaStyle.s52Lookup.has_value());
  REQUIRE(depthAreaStyle.s52Lookup->instructions.size() == 2);
  REQUIRE(instructionAssetId(depthAreaStyle.s52Lookup->instructions.front()) == "DEPARE01");
  REQUIRE(genericAreaStyle.styleKey == "area/default");
  REQUIRE(genericAreaStyle.textKey.empty());
  REQUIRE_FALSE(genericAreaStyle.s52Lookup.has_value());
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

  const auto buoyStyle = symbolizer.symbolize(buoy);
  REQUIRE(buoyStyle.s52Lookup.has_value());
  REQUIRE_FALSE(buoyStyle.suppressed);
  REQUIRE(instructionAssetId(buoyStyle.s52Lookup->instructions.front()) == "BOYSPP02");

  const auto wreckStyle = symbolizer.symbolize(namedWreck);
  REQUIRE(wreckStyle.s52Lookup.has_value());
  REQUIRE_FALSE(wreckStyle.suppressed);
  REQUIRE(wreckStyle.s52Lookup->instructions.size() == 1);
  REQUIRE(wreckStyle.textKey.empty());
}
