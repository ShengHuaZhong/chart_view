#include <catch2/catch_test_macros.hpp>

#include "chart_data/geometry.hpp"
#include "chart_data/dataset_meta.hpp"
#include "chart_data/feature.hpp"
#include "chart_data/feature_chart_dataset.hpp"

using namespace chart_view::runtime::chart_data;

// ---------- Geometry ----------

TEST_CASE("PointGeometry stores a coordinate", "[chart_data][geometry]")
{
  PointGeometry pt{{121.5, 31.2}};
  REQUIRE(pt.position.lon == 121.5);
  REQUIRE(pt.position.lat == 31.2);
}

TEST_CASE("LineGeometry stores vertices", "[chart_data][geometry]")
{
  LineGeometry line;
  line.vertices = {{0.0, 0.0}, {1.0, 1.0}, {2.0, 0.0}};
  REQUIRE(line.vertices.size() == 3);
}

TEST_CASE("AreaGeometry has exterior and interior rings", "[chart_data][geometry]")
{
  AreaGeometry area;
  area.exteriorRing = {{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}};
  area.interiorRings.push_back({{2.0, 2.0}, {4.0, 2.0}, {4.0, 4.0}, {2.0, 4.0}});
  REQUIRE(area.exteriorRing.size() == 4);
  REQUIRE(area.interiorRings.size() == 1);
}

TEST_CASE("Geometry variant and geometryType()", "[chart_data][geometry]")
{
  Geometry gPt = PointGeometry{{0.0, 0.0}};
  REQUIRE(geometryType(gPt) == GeometryType::kPoint);

  Geometry gLine = LineGeometry{{{0.0, 0.0}, {1.0, 1.0}}};
  REQUIRE(geometryType(gLine) == GeometryType::kLine);

  Geometry gArea = AreaGeometry{{{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}}, {}};
  REQUIRE(geometryType(gArea) == GeometryType::kArea);
}

TEST_CASE("Extent validity check", "[chart_data][geometry]")
{
  Extent valid{0.0, 0.0, 10.0, 10.0};
  REQUIRE(valid.isValid());

  Extent invalid{10.0, 0.0, 0.0, 10.0};
  REQUIRE_FALSE(invalid.isValid());
}

// ---------- DatasetMeta ----------

TEST_CASE("DatasetMeta default values", "[chart_data][meta]")
{
  DatasetMeta meta;
  REQUIRE(meta.name.empty());
  REQUIRE(meta.sourceType == chart_view_chart_source_unknown);
  REQUIRE(meta.nativeScale == 0.0);
}

// ---------- Feature ----------

TEST_CASE("Feature with point geometry and attributes", "[chart_data][feature]")
{
  Feature f;
  f.id = 42;
  f.classCode = 42;
  f.classAcronym = "SOUNDG";
  f.geometry = PointGeometry{{120.0, 30.0}};
  f.attributes["DRVAL1"] = 12.5;
  f.attributes["QUASOU"] = std::int64_t{6};

  REQUIRE(f.id == 42);
  REQUIRE(f.classAcronym == "SOUNDG");
  REQUIRE(geometryType(f.geometry) == GeometryType::kPoint);
  REQUIRE(std::get<double>(f.attributes.at("DRVAL1")) == 12.5);
  REQUIRE(std::get<std::int64_t>(f.attributes.at("QUASOU")) == 6);
}

// ---------- FeatureChartDataset ----------

TEST_CASE("FeatureChartDataset starts empty", "[chart_data][dataset]")
{
  FeatureChartDataset ds;
  REQUIRE(ds.empty());
  REQUIRE(ds.featureCount() == 0);
  REQUIRE(ds.meta().sourceType == chart_view_chart_source_unknown);
}

TEST_CASE("FeatureChartDataset accepts S57-like data", "[chart_data][dataset]")
{
  FeatureChartDataset ds;
  DatasetMeta meta;
  meta.name = "US5WA51M";
  meta.sourceType = chart_view_chart_source_s57;
  meta.nativeScale = 50000.0;
  meta.extent = {-123.0, 47.0, -122.0, 48.0};
  ds.setMeta(std::move(meta));

  Feature depare;
  depare.id = 1;
  depare.classCode = 42;
  depare.classAcronym = "DEPARE";
  depare.geometry = AreaGeometry{
    {{-123.0, 47.0}, {-122.0, 47.0}, {-122.0, 48.0}, {-123.0, 48.0}}, {}};
  depare.attributes["DRVAL1"] = 0.0;
  depare.attributes["DRVAL2"] = 20.0;
  ds.addFeature(std::move(depare));

  Feature soundg;
  soundg.id = 2;
  soundg.classCode = 129;
  soundg.classAcronym = "SOUNDG";
  soundg.geometry = PointGeometry{{-122.5, 47.5}};
  soundg.attributes["VALSOU"] = 15.3;
  ds.addFeature(std::move(soundg));

  REQUIRE(ds.featureCount() == 2);
  REQUIRE(ds.meta().sourceType == chart_view_chart_source_s57);
  REQUIRE(ds.features()[0].classAcronym == "DEPARE");
  REQUIRE(ds.features()[1].classAcronym == "SOUNDG");
}

TEST_CASE("FeatureChartDataset accepts CM93-like data", "[chart_data][dataset]")
{
  FeatureChartDataset ds;
  DatasetMeta meta;
  meta.name = "CM93_CELL";
  meta.sourceType = chart_view_chart_source_cm93;
  ds.setMeta(std::move(meta));

  Feature coalne;
  coalne.id = 100;
  coalne.classCode = 71;
  coalne.classAcronym = "COALNE";
  coalne.geometry = LineGeometry{{{0.0, 0.0}, {1.0, 0.5}, {2.0, 0.0}}};
  ds.addFeature(std::move(coalne));

  REQUIRE(ds.featureCount() == 1);
  REQUIRE(ds.meta().sourceType == chart_view_chart_source_cm93);
}

TEST_CASE("FeatureChartDataset accepts S-101-like data", "[chart_data][dataset]")
{
  FeatureChartDataset ds;
  DatasetMeta meta;
  meta.name = "101US00001";
  meta.sourceType = chart_view_chart_source_s101;
  meta.nativeScale = 22000.0;
  meta.extent = {-76.0, 38.0, -75.0, 39.0};
  ds.setMeta(std::move(meta));

  Feature f;
  f.id = 1;
  f.classCode = 1;
  f.classAcronym = "DepthArea";
  f.geometry = AreaGeometry{
    {{-76.0, 38.0}, {-75.0, 38.0}, {-75.0, 39.0}, {-76.0, 39.0}}, {}};
  f.attributes["minimumDepth"] = 5.0;
  ds.addFeature(std::move(f));

  REQUIRE(ds.featureCount() == 1);
  REQUIRE(ds.meta().sourceType == chart_view_chart_source_s101);
  REQUIRE(ds.features()[0].classAcronym == "DepthArea");
}

TEST_CASE("FeatureChartDataset clear and reserve", "[chart_data][dataset]")
{
  FeatureChartDataset ds;
  ds.reserveFeatures(100);
  Feature f;
  f.id = 1;
  f.geometry = PointGeometry{{0.0, 0.0}};
  ds.addFeature(std::move(f));
  REQUIRE(ds.featureCount() == 1);

  ds.clearFeatures();
  REQUIRE(ds.empty());
}
