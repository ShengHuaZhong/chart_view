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

  REQUIRE(symbolizer.symbolize(wreck).styleKey == "point/danger");
  REQUIRE(symbolizer.symbolize(fairway).styleKey == "line/channel");
  REQUIRE(symbolizer.symbolize(landArea).styleKey == "area/land");
  REQUIRE(symbolizer.symbolize(restrictedArea).styleKey == "area/restricted");
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
  REQUIRE(symbolizer.symbolize(fairway).styleKey == "line/channel");
  REQUIRE(symbolizer.symbolize(landArea).styleKey == "area/land");
  REQUIRE(symbolizer.symbolize(restrictedArea).styleKey == "area/restricted");
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
  REQUIRE(buoyStyle.styleKey == "point/buoy");
  REQUIRE(beaconStyle.styleKey == "point/beacon");
  REQUIRE(genericStyle.styleKey == "point/default");
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

  REQUIRE(symbolizer.symbolize(depthContour).styleKey == "line/depth_contour");
  REQUIRE(symbolizer.symbolize(coastline).styleKey == "line/coastline");
  REQUIRE(symbolizer.symbolize(defaultLine).styleKey == "line/default");
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
  REQUIRE(genericAreaStyle.styleKey == "area/default");
  REQUIRE(genericAreaStyle.textKey.empty());
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
  meta.sourceType = chart_view_chart_source_s101;
  meta.extent = {121.0, 31.0, 121.3, 31.3};
  dataset.setMeta(std::move(meta));

  Feature sounding;
  sounding.id = 1;
  sounding.geometry = PointGeometry{{121.0, 31.0}};
  sounding.attributes["VALSOU"] = 12.5;
  dataset.addFeature(std::move(sounding));

  Feature coastline;
  coastline.id = 2;
  coastline.geometry = LineGeometry{{{121.0, 31.0}, {121.2, 31.2}}};
  coastline.attributes["CATCOA"] = std::int64_t{1};
  dataset.addFeature(std::move(coastline));

  Feature depthArea;
  depthArea.id = 3;
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
  REQUIRE(symbolizer.symbolize(readback.dataset.features()[0]).styleKey == "point/sounding");
  REQUIRE(symbolizer.symbolize(readback.dataset.features()[1]).styleKey == "line/coastline");
  const auto areaStyle = symbolizer.symbolize(readback.dataset.features()[2]);
  REQUIRE(areaStyle.styleKey == "area/depth");
  REQUIRE(areaStyle.textKey == "text/default");
}
