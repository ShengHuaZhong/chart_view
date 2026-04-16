#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "chart_data/dataset_meta.hpp"
#include "chart_data/feature.hpp"
#include "chart_data/feature_chart_dataset.hpp"
#include "chart_data/geometry.hpp"
#include "s57/s57_source_model.hpp"
#include "senc/senc_reader.hpp"
#include "senc/senc_types.hpp"
#include "senc/senc_writer.hpp"

#include <cstring>
#include <string>
#include <vector>

using namespace chart_view::runtime::chart_data;
using namespace chart_view::runtime::s57;
using namespace chart_view::runtime::senc;

namespace {

std::uint64_t makeIdentityDerivedId(std::uint16_t agency, std::uint32_t featureId, std::uint16_t subdivision)
{
  return (static_cast<std::uint64_t>(agency) << 48) |
         (static_cast<std::uint64_t>(featureId) << 16) |
         subdivision;
}

FeatureChartDataset makeSyntheticDataset()
{
  FeatureChartDataset dataset;

  DatasetMeta meta;
  meta.name = "US5TX01M";
  meta.sourceType = chart_view_chart_source_s57;
  meta.nativeScale = 50000.0;
  meta.extent = Extent{-97.5, 27.5, -96.0, 28.5};
  meta.usageBand = 4;
  meta.edition = 12;
  meta.update = 2;
  dataset.setMeta(meta);

  Feature point;
  point.id = 101;
  point.classCode = 86;
  point.classAcronym = "LIGHTS";
  point.geometry = PointGeometry{{-96.5, 28.0}};
  point.attributes["OBJNAM"] = std::string("Harbor Light");
  point.attributes["CATLIT"] = std::int64_t{1};
  dataset.addFeature(std::move(point));

  Feature line;
  line.id = makeIdentityDerivedId(550, 42, 0);
  line.classCode = 71;
  line.classAcronym = "COALNE";
  line.geometry = LineGeometry{{{-97.0, 27.5}, {-96.5, 28.0}, {-96.0, 28.5}}};
  line.attributes["OBJNAM"] = std::string("Coastline");
  line.attributes["CATCOA"] = std::int64_t{1};
  dataset.addFeature(std::move(line));

  return dataset;
}

S57SourceModel makeSyntheticSourceModel()
{
  S57SourceModel model;
  model.sourcePath = "C:/charts/US5TX01M.000";
  model.sourceName = "US5TX01M.000";
  model.declaredDatasetName = "US5TX01M";
  model.coordinateMultiplier = 1.0e-7;
  model.datasetMeta = makeSyntheticDataset().meta();
  model.sourceManifest.name = "US5TX01M.000";
  model.sourceManifest.sourceType = chart_view_chart_source_s57;
  model.sourceManifest.sourceSize = 2048;
  model.sourceManifest.sourceTimestamp = 1700000000;
  model.sourceManifest.sourceHash = 0x12345678;
  model.sourceManifest.edition = 12;
  model.sourceManifest.update = 2;

  model.updateManifest.baseName = "US5TX01M.000";
  model.updateManifest.edition = 12;
  model.updateManifest.baseUpdate = 0;
  model.updateManifest.highestContiguousUpdate = 2;
  model.updateManifest.lastAppliedUpdate = 2;
  model.updateManifest.nextMissingUpdate = 3;
  model.updateManifest.appliedUpdates = {1, 2};
  model.updateManifest.availableUpdates = {
    {"US5TX01M.001", "US5TX01M.001", 1, 128, 1700000100},
    {"US5TX01M.002", "US5TX01M.002", 2, 256, 1700000200},
  };

  S57SourceVectorRecord pointVector;
  pointVector.recordName = 110;
  pointVector.recordId = 1;
  pointVector.recordVersion = 2;
  pointVector.updateInstruction = 3;
  pointVector.coords = {{-96.5, 28.0}};
  model.vectors.insert_or_assign((static_cast<std::uint64_t>(pointVector.recordName) << 32) | pointVector.recordId, pointVector);

  S57SourceVectorRecord lineVector;
  lineVector.recordName = 120;
  lineVector.recordId = 2;
  lineVector.recordVersion = 3;
  lineVector.updateInstruction = 3;
  lineVector.coords = {{-97.0, 27.5}, {-96.5, 28.0}, {-96.0, 28.5}};
  model.vectors.insert_or_assign((static_cast<std::uint64_t>(lineVector.recordName) << 32) | lineVector.recordId, lineVector);

  S57SourceFeature pointFeature;
  pointFeature.recordId = 101;
  pointFeature.recordVersion = 2;
  pointFeature.updateInstruction = 3;
  pointFeature.primitive = S57Primitive::kPoint;
  pointFeature.classCode = 86;
  pointFeature.classAcronym = "LIGHTS";
  pointFeature.attributes["OBJNAM"] = std::string("Harbor Light");
  pointFeature.attributes["CATLIT"] = std::int64_t{1};
  pointFeature.nationalAttributes["NOBJNM"] = std::string("Harbor Light CN");
  pointFeature.spatialPointers.push_back({110, 1, 1, 1, 0});
  pointFeature.geometry = PointGeometry{{-96.5, 28.0}};
  model.features.push_back(pointFeature);

  S57SourceFeature lineFeature;
  lineFeature.recordId = 202;
  lineFeature.recordVersion = 3;
  lineFeature.updateInstruction = 3;
  lineFeature.primitive = S57Primitive::kLine;
  lineFeature.classCode = 71;
  lineFeature.classAcronym = "COALNE";
  lineFeature.identity = S57FeatureIdentity{550, 42, 0};
  lineFeature.attributes["OBJNAM"] = std::string("Coastline");
  lineFeature.attributes["CATCOA"] = std::int64_t{1};
  lineFeature.nationalAttributes["NATF_TEST"] = std::string("National Coast");
  lineFeature.spatialPointers.push_back({120, 2, 1, 1, 0});
  lineFeature.geometry = LineGeometry{{{-97.0, 27.5}, {-96.5, 28.0}, {-96.0, 28.5}}};
  model.features.push_back(lineFeature);

  return model;
}

} // namespace

TEST_CASE("Senc v2 roundtrip preserves S57 semantic payload and update manifest", "[senc][v2]")
{
  auto dataset = makeSyntheticDataset();
  auto sourceModel = makeSyntheticSourceModel();

  SencWriter writer;
  writer.setFormatVersion(kSencFormatVersionV2);
  writer.setSourceManifest(sourceModel.sourceManifest);
  writer.setS57SourceModel(sourceModel);

  const auto blob = writer.write(dataset);
  REQUIRE_FALSE(blob.empty());

  FileHeader header{};
  std::memcpy(&header, blob.data(), sizeof(header));
  REQUIRE(header.formatVersion == kSencFormatVersionV2);

  SencReader reader;
  const auto readResult = reader.read(blob);
  REQUIRE(readResult.ok);
  REQUIRE(readResult.manifest.has_value());
  REQUIRE(readResult.sourceModel.has_value());

  const auto &semantic = readResult.sourceModel.value();
  REQUIRE(semantic.datasetMeta.name == "US5TX01M");
  REQUIRE(semantic.sourceManifest.name == "US5TX01M.000");
  REQUIRE(semantic.sourceManifest.update == 2);
  REQUIRE(semantic.updateManifest.appliedUpdates == std::vector<std::uint32_t>{1, 2});
  REQUIRE(semantic.updateManifest.lastAppliedUpdate == 2);
  REQUIRE(semantic.updateManifest.nextMissingUpdate == 3);
  REQUIRE(semantic.updateManifest.availableUpdates.size() == 2);
  CHECK(semantic.updateManifest.availableUpdates[0].name == "US5TX01M.001");
  CHECK(semantic.coordinateMultiplier == Catch::Approx(1.0e-7));

  REQUIRE(semantic.vectors.size() == 2);
  REQUIRE(semantic.features.size() == 2);

  const auto &light = semantic.features[0];
  CHECK(light.classAcronym == "LIGHTS");
  CHECK(light.recordId == 101);
  REQUIRE(light.attributes.contains("OBJNAM"));
  CHECK(std::get<std::string>(light.attributes.at("OBJNAM")) == "Harbor Light");
  REQUIRE(light.nationalAttributes.contains("NOBJNM"));
  CHECK(std::get<std::string>(light.nationalAttributes.at("NOBJNM")) == "Harbor Light CN");
  REQUIRE(light.spatialPointers.size() == 1);
  CHECK(geometryType(light.geometry) == GeometryType::kPoint);

  const auto &coast = semantic.features[1];
  CHECK(coast.classAcronym == "COALNE");
  REQUIRE(coast.identity.has_value());
  CHECK(coast.identity->agency == 550);
  CHECK(coast.identity->featureId == 42);
  CHECK(geometryType(coast.geometry) == GeometryType::kLine);
}

TEST_CASE("Senc v1 roundtrip remains available without semantic payload", "[senc][v2]")
{
  auto dataset = makeSyntheticDataset();

  SencWriter writer;
  const auto blob = writer.write(dataset);

  SencReader reader;
  const auto readResult = reader.read(blob);
  REQUIRE(readResult.ok);
  CHECK_FALSE(readResult.sourceModel.has_value());
  CHECK(readResult.dataset.featureCount() == dataset.featureCount());
}
