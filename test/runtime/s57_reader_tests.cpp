#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "s57/iso8211.hpp"
#include "s57/s57_attribute_codec.hpp"
#include "s57/s57_reader.hpp"
#include "s57/s57_normalizer.hpp"
#include "s57/s57_semantic_mapping.hpp"
#include "s57/s57_source_model.hpp"
#include "s57/s57_update_application.hpp"
#include "chart_data/geometry.hpp"
#include "chart_data/feature.hpp"
#include "chart_data/feature_chart_dataset.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace chart_view::runtime;
using namespace chart_view::runtime::s57;

// ------------------------------------------------------------------
// Helper: get the test data root from env, skip if not available.
// ------------------------------------------------------------------
static std::string getS57Root()
{
#ifdef CHARTSYS_S57_TESTDATA_ROOT
  const char *root = CHARTSYS_S57_TESTDATA_ROOT;
#else
  const char *root = std::getenv("CHARTSYS_S57_TESTDATA_ROOT");
#endif
  if (!root || !std::filesystem::exists(root)) {
    SKIP("CHARTSYS_S57_TESTDATA_ROOT not set or missing");
  }
  return root;
}

// Find first .000 file in the test data root.
static std::string findFirstChart(const std::string &root)
{
  std::string result;
  for (const auto &entry : std::filesystem::directory_iterator(root)) {
    if (entry.path().extension() == ".000") {
      result = entry.path().string();
      break;
    }
  }
  if (result.empty()) {
    FAIL("No .000 file found in " + root);
  }
  return result;
}

static std::string findChartByStem(const std::string &root, std::string_view stem)
{
  for(const auto &entry : std::filesystem::directory_iterator(root)) {
    if(entry.path().extension() == ".000" && entry.path().stem().string() == stem) {
      return entry.path().string();
    }
  }

  return {};
}

static void writeBytes(const std::filesystem::path &path, std::span<const std::uint8_t> bytes)
{
  std::ofstream stream(path, std::ios::binary);
  REQUIRE(stream.good());
  if(!bytes.empty()) {
    stream.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  }
  REQUIRE(stream.good());
}

static S57SourceVectorRecord makeVectorRecord(
  std::uint8_t recordName,
  std::uint32_t recordId,
  std::initializer_list<chart_data::Coordinate> coords,
  std::uint8_t ruin = 0,
  std::uint16_t version = 1)
{
  S57SourceVectorRecord record;
  record.recordName = recordName;
  record.recordId = recordId;
  record.recordVersion = version;
  record.updateInstruction = ruin;
  record.coords.assign(coords.begin(), coords.end());
  return record;
}

static S57SourceFeature makeFeature(
  std::uint32_t recordId,
  S57Primitive primitive,
  std::uint32_t classCode,
  std::string classAcronym,
  std::uint8_t ruin = 0,
  std::uint16_t version = 1)
{
  S57SourceFeature feature;
  feature.recordId = recordId;
  feature.recordVersion = version;
  feature.updateInstruction = ruin;
  feature.primitive = primitive;
  feature.classCode = classCode;
  feature.classAcronym = std::move(classAcronym);
  feature.identity = S57FeatureIdentity{540, recordId, 0};
  return feature;
}

// ==================================================================
// ISO 8211 parser unit tests
// ==================================================================

TEST_CASE("iso8211::parse rejects empty data", "[s57][iso8211]")
{
  std::vector<std::uint8_t> empty;
  auto result = iso8211::parse(empty);
  REQUIRE_FALSE(result.ok);
  REQUIRE_THAT(result.error, Catch::Matchers::ContainsSubstring("too small"));
}

TEST_CASE("iso8211::parse rejects truncated data", "[s57][iso8211]")
{
  std::vector<std::uint8_t> data(20, 0x20); // 20 spaces
  auto result = iso8211::parse(data);
  REQUIRE_FALSE(result.ok);
}

// ==================================================================
// S57Reader smoke tests on real chart data
// ==================================================================

TEST_CASE("S57Reader reads canonical S57 chart", "[s57][real-data]")
{
  auto root = getS57Root();
  auto chartPath = findFirstChart(root);

  S57Reader reader;
  auto result = reader.read(chartPath);

  REQUIRE(result.ok);
  REQUIRE(result.error.empty());

  const auto &ds = result.dataset;
  INFO("chart: " << chartPath);
  INFO("features: " << ds.featureCount());
  INFO("name: " << ds.meta().name);

  // Non-zero feature count.
  REQUIRE(ds.featureCount() > 0);

  // Dataset extent must be valid.
  const auto &ext = ds.meta().extent;
  REQUIRE(ext.isValid());

  // Extent should be in reasonable geographic range.
  REQUIRE(ext.minLon >= -180.0);
  REQUIRE(ext.maxLon <= 180.0);
  REQUIRE(ext.minLat >= -90.0);
  REQUIRE(ext.maxLat <= 90.0);

  // Source type must be S-57.
  REQUIRE(ds.meta().sourceType == chart_view_chart_source_s57);

  REQUIRE(result.sourceModel.features.size() == ds.featureCount());
  REQUIRE(result.sourceModel.datasetMeta.name == ds.meta().name);
  REQUIRE(result.sourceModel.datasetMeta.extent.isValid());
  REQUIRE(result.sourceModel.coordinateMultiplier > 0.0);
  REQUIRE(result.sourceModel.sourceManifest.sourceType == chart_view_chart_source_s57);
  REQUIRE(result.sourceModel.sourceManifest.name == std::filesystem::path(chartPath).filename().string());
  REQUIRE(result.sourceModel.sourceManifest.sourceSize > 0);
  REQUIRE(result.sourceModel.sourceManifest.sourceHash != 0);
  REQUIRE(result.sourceModel.updateManifest.baseName == std::filesystem::path(chartPath).filename().string());
  REQUIRE(result.sourceModel.updateManifest.availableUpdates.empty());
  REQUIRE(result.sourceModel.updateManifest.lastAppliedUpdate == ds.meta().update);
  REQUIRE(result.sourceModel.updateManifest.highestContiguousUpdate == ds.meta().update);
  REQUIRE(result.sourceModel.updateManifest.nextMissingUpdate == ds.meta().update + 1);
  REQUIRE_FALSE(result.sourceModel.declaredDatasetName.empty());

  std::size_t featuresWithIdentity = 0;
  for(const auto &feature : result.sourceModel.features) {
    if(feature.identity.has_value() && feature.identity->isValid()) {
      ++featuresWithIdentity;
    }
  }
  INFO("featuresWithIdentity: " << featuresWithIdentity);
  REQUIRE(featuresWithIdentity > 0);
}

TEST_CASE("S57Reader extracts point, line, area features", "[s57][real-data]")
{
  auto root = getS57Root();
  auto chartPath = findFirstChart(root);

  S57Reader reader;
  auto result = reader.read(chartPath);
  REQUIRE(result.ok);

  std::size_t pointCount = 0;
  std::size_t lineCount = 0;
  std::size_t areaCount = 0;

  for (const auto &feat : result.dataset.features()) {
    std::visit([&](const auto &geo) {
      using T = std::decay_t<decltype(geo)>;
      if constexpr (std::is_same_v<T, chart_data::PointGeometry>) {
        ++pointCount;
      } else if constexpr (std::is_same_v<T, chart_data::LineGeometry>) {
        ++lineCount;
      } else if constexpr (std::is_same_v<T, chart_data::AreaGeometry>) {
        ++areaCount;
      }
    }, feat.geometry);
  }

  INFO("points: " << pointCount << " lines: " << lineCount << " areas: " << areaCount);

  // At least some features of each type should exist in a typical ENC chart.
  // But we require at least one category to be non-empty.
  REQUIRE((pointCount + lineCount + areaCount) > 0);
}

TEST_CASE("S57Reader features have attributes", "[s57][real-data]")
{
  auto root = getS57Root();
  auto chartPath = findFirstChart(root);

  S57Reader reader;
  auto result = reader.read(chartPath);
  REQUIRE(result.ok);

  // At least some features should have attributes.
  std::size_t withAttrs = 0;
  for (const auto &feat : result.dataset.features()) {
    if (!feat.attributes.empty()) ++withAttrs;
  }

  INFO("features with attributes: " << withAttrs << " / " << result.dataset.featureCount());
  REQUIRE(withAttrs > 0);
}

TEST_CASE("S57 semantic mapping exposes the Phase 4 baseline acronyms", "[s57][semantic]")
{
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(30) == "COALNE");
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(42) == "DEPARE");
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(43) == "DEPCNT");
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(75) == "LIGHTS");
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(129) == "SOUNDG");
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(159) == "WRECKS");

  REQUIRE(chart_view::runtime::s57::lookupAttributeAcronym(116) == "OBJNAM");
  REQUIRE(chart_view::runtime::s57::lookupAttributeAcronym(301) == "NOBJNM");
  REQUIRE(chart_view::runtime::s57::lookupAttributeAcronym(174) == "VALDCO");
  REQUIRE(chart_view::runtime::s57::lookupAttributeAcronym(179) == "VALSOU");
}

TEST_CASE("S57 semantic mapping exposes broader Phase 5 acronym coverage", "[s57][semantic][phase5]")
{
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(4) == "ACHARE");
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(11) == "BRIDGE");
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(64) == "HRBFAC");
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(86) == "OBSTRN");
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(122) == "SLCONS");
  REQUIRE(chart_view::runtime::s57::lookupObjectClassAcronym(146) == "TSSBND");

  REQUIRE(chart_view::runtime::s57::lookupAttributeAcronym(8) == "CATACH");
  REQUIRE(chart_view::runtime::s57::lookupAttributeAcronym(37) == "CATLIT");
  REQUIRE(chart_view::runtime::s57::lookupAttributeAcronym(102) == "INFORM");
  REQUIRE(chart_view::runtime::s57::lookupAttributeAcronym(131) == "RESTRN");
  REQUIRE(chart_view::runtime::s57::lookupAttributeAcronym(133) == "SCAMIN");
  REQUIRE(chart_view::runtime::s57::lookupAttributeAcronym(149) == "STATUS");
  REQUIRE(chart_view::runtime::s57::lookupAttributeAcronym(178) == "VALNMR");
  REQUIRE(chart_view::runtime::s57::lookupAttributeAcronym(300) == "NINFOM");
}

TEST_CASE("S57 attribute codec decodes list values and merges repeated attributes", "[s57][semantic][phase5]")
{
  using chart_view::runtime::chart_data::AttributeDoubleList;
  using chart_view::runtime::chart_data::AttributeIntList;
  using chart_view::runtime::chart_data::AttributeStringList;

  const auto integralList = chart_view::runtime::s57::decodeS57AttributeValue("1, 2, 3");
  REQUIRE(std::holds_alternative<AttributeIntList>(integralList));
  CHECK(std::get<AttributeIntList>(integralList) == AttributeIntList{1, 2, 3});

  const auto numericList = chart_view::runtime::s57::decodeS57AttributeValue("1.5,2,3.5");
  REQUIRE(std::holds_alternative<AttributeDoubleList>(numericList));
  CHECK(std::get<AttributeDoubleList>(numericList).size() == 3);
  CHECK(std::get<AttributeDoubleList>(numericList)[0] == Catch::Approx(1.5));
  CHECK(std::get<AttributeDoubleList>(numericList)[1] == Catch::Approx(2.0));
  CHECK(std::get<AttributeDoubleList>(numericList)[2] == Catch::Approx(3.5));

  const auto stringList = chart_view::runtime::s57::decodeS57AttributeValue("north; south ; east");
  REQUIRE(std::holds_alternative<AttributeStringList>(stringList));
  CHECK(std::get<AttributeStringList>(stringList)
        == AttributeStringList{"north", "south", "east"});

  chart_view::runtime::s57::S57AttributeMap attributes;
  chart_view::runtime::s57::mergeS57AttributeValue(attributes, "CATACH", std::int64_t{2});
  chart_view::runtime::s57::mergeS57AttributeValue(attributes, "CATACH", std::int64_t{6});
  REQUIRE(attributes.contains("CATACH"));
  REQUIRE(std::holds_alternative<AttributeIntList>(attributes.at("CATACH")));
  CHECK(std::get<AttributeIntList>(attributes.at("CATACH")) == AttributeIntList{2, 6});

  chart_view::runtime::s57::mergeS57AttributeValue(attributes, "VALDCO", 10.0);
  chart_view::runtime::s57::mergeS57AttributeValue(attributes, "VALDCO", std::int64_t{12});
  REQUIRE(std::holds_alternative<AttributeDoubleList>(attributes.at("VALDCO")));
  CHECK(std::get<AttributeDoubleList>(attributes.at("VALDCO")) == AttributeDoubleList{10.0, 12.0});

  chart_view::runtime::s57::mergeS57AttributeValue(attributes, "INFORM", std::string("north"));
  chart_view::runtime::s57::mergeS57AttributeValue(attributes, "INFORM", std::string("channel"));
  REQUIRE(std::holds_alternative<AttributeStringList>(attributes.at("INFORM")));
  CHECK(std::get<AttributeStringList>(attributes.at("INFORM"))
        == AttributeStringList{"north", "channel"});
}

TEST_CASE("S57Reader preserves baseline semantic names for the known real pair", "[s57][real-data][semantic][phase4]")
{
  const auto root = getS57Root();
  const auto chartAPath = findChartByStem(root, "C1511781");
  const auto chartBPath = findChartByStem(root, "C1511782");
  if(chartAPath.empty() || chartBPath.empty()) {
    SKIP("Known real-pair charts C1511781/C1511782 not available");
  }

  S57Reader reader;
  const auto chartA = reader.read(chartAPath);
  const auto chartB = reader.read(chartBPath);
  REQUIRE(chartA.ok);
  REQUIRE(chartB.ok);

  std::size_t mappedClassCount = 0;
  std::size_t namedFeatureCount = 0;
  std::size_t nationalNameCount = 0;
  for(const auto *dataset : {&chartA.dataset, &chartB.dataset}) {
    for(const auto &feature : dataset->features()) {
      if(!feature.classAcronym.empty() && !feature.classAcronym.starts_with("OBJ")) {
        ++mappedClassCount;
      }

      if(feature.attributes.contains("OBJNAM")) {
        ++namedFeatureCount;
      }
      if(feature.attributes.contains("NOBJNM")) {
        ++nationalNameCount;
      }
    }
  }

  INFO("mappedClassCount=" << mappedClassCount);
  INFO("namedFeatureCount=" << namedFeatureCount);
  INFO("nationalNameCount=" << nationalNameCount);

  REQUIRE(mappedClassCount > 0);
  REQUIRE(namedFeatureCount > 0);
}

TEST_CASE("S57 source manifest can be built for in-memory buffers", "[s57][source-model]")
{
  const std::vector<std::uint8_t> bytes{0x01, 0x02, 0x03, 0x04, 0x05};
  const auto manifest = buildSourceManifestForBuffer("DEMO.000", bytes, 7, 3);

  REQUIRE(manifest.name == "DEMO.000");
  REQUIRE(manifest.sourceType == chart_view_chart_source_s57);
  REQUIRE(manifest.sourceSize == bytes.size());
  REQUIRE(manifest.sourceHash != 0);
  REQUIRE(manifest.edition == 7);
  REQUIRE(manifest.update == 3);
}

TEST_CASE("S57 update manifest detects gaps in sequential update files", "[s57][source-model]")
{
  const auto uniqueSuffix =
    std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  const auto root = std::filesystem::temp_directory_path() /
                    ("chart_view_s57_update_manifest_" + uniqueSuffix);
  std::filesystem::create_directories(root);

  const auto basePath = root / "TESTCHART.000";
  const std::vector<std::uint8_t> bytes{0x41, 0x42, 0x43};
  writeBytes(basePath, bytes);
  writeBytes(root / "TESTCHART.001", bytes);
  writeBytes(root / "TESTCHART.002", bytes);
  writeBytes(root / "TESTCHART.004", bytes);
  writeBytes(root / "TESTCHART.TXT", bytes);

  const auto manifest = buildUpdateManifestForBasePath(basePath.string(), 11, 0);

  REQUIRE(manifest.basePath == basePath.string());
  REQUIRE(manifest.baseName == "TESTCHART.000");
  REQUIRE(manifest.edition == 11);
  REQUIRE(manifest.baseUpdate == 0);
  REQUIRE(manifest.availableUpdates.size() == 3);
  REQUIRE(manifest.availableUpdates[0].updateNumber == 1);
  REQUIRE(manifest.availableUpdates[1].updateNumber == 2);
  REQUIRE(manifest.availableUpdates[2].updateNumber == 4);
  REQUIRE(manifest.highestContiguousUpdate == 2);
  REQUIRE(manifest.lastAppliedUpdate == 0);
  REQUIRE(manifest.nextMissingUpdate == 3);
  REQUIRE(manifest.hasPendingUpdates());
  REQUIRE_FALSE(manifest.hasAppliedUpdates());

  std::filesystem::remove_all(root);
}

TEST_CASE("S57 update application applies sequential vector and feature updates", "[s57][updates]")
{
  S57SourceModel base;
  base.sourceName = "BASE.000";
  base.datasetMeta.name = "BASE";
  base.datasetMeta.sourceType = chart_view_chart_source_s57;
  base.datasetMeta.edition = 1;
  base.datasetMeta.update = 0;
  base.sourceManifest.name = "BASE.000";
  base.sourceManifest.sourceType = chart_view_chart_source_s57;
  base.sourceManifest.edition = 1;
  base.sourceManifest.update = 0;
  base.updateManifest.baseName = "BASE.000";
  base.updateManifest.baseUpdate = 0;
  base.updateManifest.lastAppliedUpdate = 0;
  base.updateManifest.highestContiguousUpdate = 2;
  base.updateManifest.nextMissingUpdate = 1;

  auto baseVector = makeVectorRecord(110, 1, {{120.0, 30.0}});
  base.vectors.emplace((static_cast<std::uint64_t>(baseVector.recordName) << 32) | baseVector.recordId, baseVector);

  auto baseFeature = makeFeature(10, S57Primitive::kPoint, 74, "LNDMRK");
  baseFeature.attributes["OBJNAM"] = std::string("Base Landmark");
  baseFeature.spatialPointers.push_back({110, 1, 0, 1, 0});
  baseFeature.geometry = chart_data::PointGeometry{{120.0, 30.0}};
  base.features.push_back(baseFeature);

  S57SourceModel update1;
  update1.datasetMeta.edition = 1;
  update1.datasetMeta.update = 1;
  update1.sourceManifest.update = 1;

  auto movedVector = makeVectorRecord(110, 1, {{121.0, 31.0}}, 3, 2);
  update1.vectors.emplace((static_cast<std::uint64_t>(movedVector.recordName) << 32) | movedVector.recordId, movedVector);
  auto insertedVector = makeVectorRecord(120, 2, {{121.0, 31.0}, {122.0, 32.0}}, 1, 1);
  update1.vectors.emplace((static_cast<std::uint64_t>(insertedVector.recordName) << 32) | insertedVector.recordId, insertedVector);

  auto modifiedFeature = makeFeature(10, S57Primitive::kPoint, 74, "LNDMRK", 3, 2);
  modifiedFeature.attributes["OBJNAM"] = std::string("Moved Landmark");
  modifiedFeature.spatialPointers.push_back({110, 1, 0, 1, 0});
  update1.features.push_back(modifiedFeature);

  auto insertedFeature = makeFeature(11, S57Primitive::kLine, 30, "COALNE", 1, 1);
  insertedFeature.attributes["OBJNAM"] = std::string("New Coastline");
  insertedFeature.spatialPointers.push_back({120, 2, 0, 1, 0});
  update1.features.push_back(insertedFeature);

  S57SourceModel update2;
  update2.datasetMeta.edition = 1;
  update2.datasetMeta.update = 2;
  update2.sourceManifest.update = 2;

  auto deletedVector = makeVectorRecord(110, 1, {}, 2, 3);
  update2.vectors.emplace((static_cast<std::uint64_t>(deletedVector.recordName) << 32) | deletedVector.recordId, deletedVector);
  auto deletedFeature = makeFeature(10, S57Primitive::kPoint, 74, "LNDMRK", 2, 3);
  update2.features.push_back(deletedFeature);

  const auto applied = applySequentialUpdates(base, {update1, update2});

  REQUIRE(applied.ok);
  REQUIRE(applied.error.empty());
  REQUIRE(applied.appliedUpdates.size() == 2);
  REQUIRE(applied.appliedUpdates[0] == 1);
  REQUIRE(applied.appliedUpdates[1] == 2);
  REQUIRE(applied.model.datasetMeta.update == 2);
  REQUIRE(applied.model.sourceManifest.update == 2);
  REQUIRE(applied.model.updateManifest.hasAppliedUpdates());
  REQUIRE(applied.model.updateManifest.lastAppliedUpdate == 2);
  REQUIRE(applied.model.updateManifest.nextMissingUpdate == 3);
  REQUIRE(applied.model.vectors.size() == 1);
  REQUIRE(applied.model.features.size() == 1);
  REQUIRE(applied.model.features.front().recordId == 11);
  REQUIRE(applied.model.features.front().attributes.contains("OBJNAM"));
  REQUIRE(std::get<std::string>(applied.model.features.front().attributes.at("OBJNAM")) == "New Coastline");

  const auto &geometry = applied.model.features.front().geometry;
  REQUIRE(std::holds_alternative<chart_data::LineGeometry>(geometry));
  const auto &line = std::get<chart_data::LineGeometry>(geometry);
  REQUIRE(line.vertices.size() == 2);
  REQUIRE(applied.model.datasetMeta.extent.isValid());
  REQUIRE(applied.model.datasetMeta.extent.minLon == Catch::Approx(121.0));
  REQUIRE(applied.model.datasetMeta.extent.maxLon == Catch::Approx(122.0));
}

TEST_CASE("S57 update application rejects missing sequential updates", "[s57][updates]")
{
  S57SourceModel base;
  base.datasetMeta.edition = 1;
  base.datasetMeta.update = 0;
  base.sourceManifest.update = 0;
  base.updateManifest.baseUpdate = 0;

  S57SourceModel update2;
  update2.datasetMeta.edition = 1;
  update2.datasetMeta.update = 2;
  update2.sourceManifest.update = 2;

  const auto applied = applySequentialUpdates(base, {update2});

  REQUIRE_FALSE(applied.ok);
  REQUIRE_THAT(applied.error, Catch::Matchers::ContainsSubstring("missing sequential update 1"));
}

TEST_CASE("S57Reader error on nonexistent file", "[s57]")
{
  S57Reader reader;
  auto result = reader.read("nonexistent_chart_file.000");
  REQUIRE_FALSE(result.ok);
  REQUIRE_THAT(result.error, Catch::Matchers::ContainsSubstring("cannot open"));
}

TEST_CASE("S57Reader error on invalid data", "[s57]")
{
  S57Reader reader;
  std::vector<std::uint8_t> garbage(100, 0xFF);
  auto result = reader.readFromMemory(garbage, "garbage");
  REQUIRE_FALSE(result.ok);
}

// ==================================================================
// S57Normalizer smoke
// ==================================================================

TEST_CASE("S57Normalizer normalizes canonical chart", "[s57][real-data]")
{
  auto root = getS57Root();
  auto chartPath = findFirstChart(root);

  S57Normalizer normalizer;
  auto result = normalizer.normalize(chartPath);

  REQUIRE(result.ok);
  REQUIRE(result.dataset.featureCount() > 0);
  REQUIRE(result.dataset.meta().extent.isValid());
}
