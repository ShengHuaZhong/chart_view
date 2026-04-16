#include <catch2/catch_test_macros.hpp>

#include "senc/senc_types.hpp"
#include "senc/senc_writer.hpp"
#include "senc/senc_reader.hpp"
#include "chart_data/feature_chart_dataset.hpp"
#include "chart_data/geometry.hpp"
#include "chart_data/dataset_meta.hpp"
#include "chart_data/feature.hpp"

#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

using namespace chart_view::runtime::senc;
using namespace chart_view::runtime::chart_data;

namespace {

FeatureChartDataset makeSyntheticDataset()
{
  FeatureChartDataset ds;

  DatasetMeta meta;
  meta.name = "US5TX01M";
  meta.sourceType = chart_view_chart_source_s57;
  meta.nativeScale = 50000.0;
  meta.extent = Extent{-97.5, 27.5, -96.0, 28.5};
  meta.usageBand = 4;
  meta.edition = 12;
  meta.update = 3;
  ds.setMeta(std::move(meta));

  // Point feature.
  Feature pt;
  pt.id = 1;
  pt.classCode = 159; // SOUNDG
  pt.classAcronym = "SOUNDG";
  pt.geometry = PointGeometry{{-96.5, 28.0}};
  pt.attributes["DRVAL1"] = 10.5;
  pt.attributes["QUASOU"] = std::int64_t{6};
  ds.addFeature(std::move(pt));

  // Line feature.
  Feature ln;
  ln.id = 2;
  ln.classCode = 71; // COALNE
  ln.classAcronym = "COALNE";
  ln.geometry = LineGeometry{{{-97.0, 27.5}, {-96.5, 28.0}, {-96.0, 28.5}}};
  ln.attributes["CATCOA"] = std::int64_t{1};
  ds.addFeature(std::move(ln));

  // Area feature.
  Feature ar;
  ar.id = 3;
  ar.classCode = 42; // DEPARE
  ar.classAcronym = "DEPARE";
  AreaGeometry areaGeo;
  areaGeo.exteriorRing = {{-97.0, 27.5}, {-96.0, 27.5}, {-96.0, 28.5}, {-97.0, 28.5}};
  areaGeo.interiorRings = {{{-96.8, 27.7}, {-96.2, 27.7}, {-96.2, 28.3}, {-96.8, 28.3}}};
  ar.geometry = std::move(areaGeo);
  ar.attributes["DRVAL1"] = 5.0;
  ar.attributes["DRVAL2"] = 20.0;
  ds.addFeature(std::move(ar));

  return ds;
}

}// namespace

TEST_CASE("SencReader rejects too-small blob", "[senc][reader]")
{
  SencReader reader;
  std::vector<std::uint8_t> tiny{0x00, 0x01};
  auto result = reader.read(tiny);
  CHECK_FALSE(result.ok);
  CHECK(result.error.find("too small") != std::string::npos);
}

TEST_CASE("SencReader rejects bad magic", "[senc][reader]")
{
  SencReader reader;
  SencWriter writer;
  auto blob = writer.write(FeatureChartDataset{});

  // Corrupt magic.
  blob[0] = 0xFF;
  auto result = reader.read(blob);
  CHECK_FALSE(result.ok);
  CHECK(result.error.find("magic") != std::string::npos);
}

TEST_CASE("SencReader rejects wrong version", "[senc][reader]")
{
  SencReader reader;
  SencWriter writer;
  auto blob = writer.write(FeatureChartDataset{});

  // Corrupt format version (bytes 4-7).
  std::uint32_t badVer = 99;
  std::memcpy(blob.data() + 4, &badVer, sizeof(badVer));
  auto result = reader.read(blob);
  CHECK_FALSE(result.ok);
  CHECK(result.error.find("version") != std::string::npos);
}

TEST_CASE("SencReader rejects corrupted CRC", "[senc][reader]")
{
  SencReader reader;
  SencWriter writer;
  auto ds = makeSyntheticDataset();
  auto blob = writer.write(ds);

  // Find a non-empty section and corrupt its payload.
  FileHeader fh{};
  std::memcpy(&fh, blob.data(), sizeof(fh));

  // DatasetMeta is at index 2.
  std::size_t descOffset = sizeof(FileHeader) + 2 * sizeof(SectionDesc);
  SectionDesc desc{};
  std::memcpy(&desc, blob.data() + descOffset, sizeof(desc));
  REQUIRE(desc.size > 0);

  // Flip a byte in the payload.
  blob[desc.offset] ^= 0xFF;

  auto result = reader.read(blob);
  CHECK_FALSE(result.ok);
  CHECK(result.error.find("CRC") != std::string::npos);
}

TEST_CASE("SencReader empty dataset roundtrip", "[senc][reader]")
{
  SencWriter writer;
  SencReader reader;

  FeatureChartDataset empty;
  auto blob = writer.write(empty);
  auto result = reader.read(blob);

  REQUIRE(result.ok);
  CHECK(result.error.empty());
  CHECK(result.dataset.featureCount() == 0);
}

TEST_CASE("SencReader full dataset metadata roundtrip", "[senc][reader]")
{
  SencWriter writer;
  SencReader reader;

  auto original = makeSyntheticDataset();
  auto blob = writer.write(original);
  auto result = reader.read(blob);

  REQUIRE(result.ok);
  const auto &meta = result.dataset.meta();
  CHECK(meta.name == "US5TX01M");
  CHECK(meta.sourceType == chart_view_chart_source_s57);
  CHECK(meta.nativeScale == 50000.0);
  CHECK(meta.extent.minLon == -97.5);
  CHECK(meta.extent.minLat == 27.5);
  CHECK(meta.extent.maxLon == -96.0);
  CHECK(meta.extent.maxLat == 28.5);
  CHECK(meta.usageBand == 4);
  CHECK(meta.edition == 12);
  CHECK(meta.update == 3);
}

TEST_CASE("SencReader full dataset feature count roundtrip", "[senc][reader]")
{
  SencWriter writer;
  SencReader reader;

  auto original = makeSyntheticDataset();
  auto blob = writer.write(original);
  auto result = reader.read(blob);

  REQUIRE(result.ok);
  REQUIRE(result.dataset.featureCount() == 3);
}

TEST_CASE("SencReader point feature roundtrip", "[senc][reader]")
{
  SencWriter writer;
  SencReader reader;

  auto original = makeSyntheticDataset();
  auto blob = writer.write(original);
  auto result = reader.read(blob);

  REQUIRE(result.ok);
  REQUIRE(result.dataset.featureCount() >= 1);

  const auto &feat = result.dataset.features()[0];
  CHECK(feat.id == 1);
  CHECK(feat.classCode == 159);
  CHECK(feat.classAcronym == "SOUNDG");
  CHECK(geometryType(feat.geometry) == GeometryType::kPoint);

  const auto &pt = std::get<PointGeometry>(feat.geometry);
  CHECK(pt.position.lon == -96.5);
  CHECK(pt.position.lat == 28.0);
}

TEST_CASE("SencReader line feature roundtrip", "[senc][reader]")
{
  SencWriter writer;
  SencReader reader;

  auto original = makeSyntheticDataset();
  auto blob = writer.write(original);
  auto result = reader.read(blob);

  REQUIRE(result.ok);
  REQUIRE(result.dataset.featureCount() >= 2);

  const auto &feat = result.dataset.features()[1];
  CHECK(feat.id == 2);
  CHECK(feat.classCode == 71);
  CHECK(feat.classAcronym == "COALNE");
  CHECK(geometryType(feat.geometry) == GeometryType::kLine);

  const auto &line = std::get<LineGeometry>(feat.geometry);
  REQUIRE(line.vertices.size() == 3);
  CHECK(line.vertices[0].lon == -97.0);
  CHECK(line.vertices[0].lat == 27.5);
  CHECK(line.vertices[2].lon == -96.0);
  CHECK(line.vertices[2].lat == 28.5);
}

TEST_CASE("SencReader area feature roundtrip", "[senc][reader]")
{
  SencWriter writer;
  SencReader reader;

  auto original = makeSyntheticDataset();
  auto blob = writer.write(original);
  auto result = reader.read(blob);

  REQUIRE(result.ok);
  REQUIRE(result.dataset.featureCount() >= 3);

  const auto &feat = result.dataset.features()[2];
  CHECK(feat.id == 3);
  CHECK(feat.classCode == 42);
  CHECK(feat.classAcronym == "DEPARE");
  CHECK(geometryType(feat.geometry) == GeometryType::kArea);

  const auto &area = std::get<AreaGeometry>(feat.geometry);
  CHECK(area.exteriorRing.size() == 4);
  REQUIRE(area.interiorRings.size() == 1);
  CHECK(area.interiorRings[0].size() == 4);
}

TEST_CASE("SencReader attribute values roundtrip", "[senc][reader]")
{
  SencWriter writer;
  SencReader reader;

  // Build a single-feature dataset with known attribute order.
  FeatureChartDataset ds;
  DatasetMeta meta;
  meta.name = "test";
  ds.setMeta(std::move(meta));

  Feature feat;
  feat.id = 42;
  feat.classCode = 1;
  feat.classAcronym = "TEST";
  feat.geometry = PointGeometry{{0.0, 0.0}};
  feat.attributes["int_attr"] = std::int64_t{-999};
  feat.attributes["dbl_attr"] = 3.14;
  feat.attributes["str_attr"] = std::string("hello");
  ds.addFeature(std::move(feat));

  auto blob = writer.write(ds);
  auto result = reader.read(blob);

  REQUIRE(result.ok);
  REQUIRE(result.dataset.featureCount() == 1);

  const auto &rf = result.dataset.features()[0];
  REQUIRE(rf.attributes.size() == 3);

  // Since unordered_map iteration order may differ, check by key.
  CHECK(std::get<std::int64_t>(rf.attributes.at("int_attr")) == -999);
  CHECK(std::get<double>(rf.attributes.at("dbl_attr")) == 3.14);
  CHECK(std::get<std::string>(rf.attributes.at("str_attr")) == "hello");
}

TEST_CASE("SencReader attribute list values roundtrip", "[senc][reader]")
{
  SencWriter writer;
  SencReader reader;

  FeatureChartDataset ds;
  DatasetMeta meta;
  meta.name = "list-test";
  ds.setMeta(std::move(meta));

  Feature feat;
  feat.id = 7;
  feat.classCode = 112;
  feat.classAcronym = "RESARE";
  feat.geometry = PointGeometry{{0.0, 0.0}};
  feat.attributes["RESTRN"] = AttributeIntList{1, 2, 7};
  feat.attributes["VALDCO"] = AttributeDoubleList{5.5, 7.0};
  feat.attributes["INFORM"] = AttributeStringList{"north", "channel"};
  ds.addFeature(std::move(feat));

  auto blob = writer.write(ds);
  auto result = reader.read(blob);

  REQUIRE(result.ok);
  REQUIRE(result.dataset.featureCount() == 1);

  const auto &attrs = result.dataset.features()[0].attributes;
  REQUIRE(std::holds_alternative<AttributeIntList>(attrs.at("RESTRN")));
  CHECK(std::get<AttributeIntList>(attrs.at("RESTRN")) == AttributeIntList{1, 2, 7});
  REQUIRE(std::holds_alternative<AttributeDoubleList>(attrs.at("VALDCO")));
  CHECK(std::get<AttributeDoubleList>(attrs.at("VALDCO")) == AttributeDoubleList{5.5, 7.0});
  REQUIRE(std::holds_alternative<AttributeStringList>(attrs.at("INFORM")));
  CHECK(std::get<AttributeStringList>(attrs.at("INFORM"))
        == AttributeStringList{"north", "channel"});
}

TEST_CASE("SencReader totalFileSize mismatch", "[senc][reader]")
{
  SencReader reader;
  SencWriter writer;
  auto blob = writer.write(FeatureChartDataset{});

  // Truncate blob.
  blob.resize(blob.size() - 1);
  auto result = reader.read(blob);
  CHECK_FALSE(result.ok);
  CHECK(result.error.find("totalFileSize") != std::string::npos);
}
