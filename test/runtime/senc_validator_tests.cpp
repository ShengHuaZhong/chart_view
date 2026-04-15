#include <catch2/catch_test_macros.hpp>

#include "senc/senc_validator.hpp"
#include "senc/senc_writer.hpp"
#include "senc/senc_reader.hpp"
#include "senc/source_manifest.hpp"
#include "chart_data/feature_chart_dataset.hpp"
#include "chart_data/dataset_meta.hpp"
#include "chart_data/feature.hpp"
#include "chart_data/geometry.hpp"

using namespace chart_view::runtime::senc;
using namespace chart_view::runtime::chart_data;

namespace {

SourceManifest makeBaseManifest()
{
  SourceManifest m;
  m.name = "US5TX01M.000";
  m.sourceType = chart_view_chart_source_s57;
  m.sourceSize = 123456;
  m.sourceTimestamp = 1700000000;
  m.sourceHash = 0xDEADBEEF;
  m.edition = 12;
  m.update = 3;
  return m;
}

}// namespace

TEST_CASE("SencValidator identical manifests -> no rebuild", "[senc][validator]")
{
  SencValidator validator;
  auto base = makeBaseManifest();
  auto result = validator.validate(base, base);
  CHECK_FALSE(result.rebuildNeeded);
  CHECK(result.reason == RebuildReason::kNone);
}

TEST_CASE("SencValidator missing SENC -> rebuild", "[senc][validator]")
{
  auto result = SencValidator::missing();
  CHECK(result.rebuildNeeded);
  CHECK(result.reason == RebuildReason::kMissing);
}

TEST_CASE("SencValidator name change -> rebuild", "[senc][validator]")
{
  SencValidator validator;
  auto stored = makeBaseManifest();
  auto current = makeBaseManifest();
  current.name = "US5TX02M.000";

  auto result = validator.validate(current, stored);
  CHECK(result.rebuildNeeded);
  CHECK(result.reason == RebuildReason::kNameMismatch);
  CHECK(result.detail.find("US5TX02M") != std::string::npos);
}

TEST_CASE("SencValidator source type change -> rebuild", "[senc][validator]")
{
  SencValidator validator;
  auto stored = makeBaseManifest();
  auto current = makeBaseManifest();
  current.sourceType = chart_view_chart_source_cm93;

  auto result = validator.validate(current, stored);
  CHECK(result.rebuildNeeded);
  CHECK(result.reason == RebuildReason::kSourceTypeMismatch);
}

TEST_CASE("SencValidator edition change -> rebuild", "[senc][validator]")
{
  SencValidator validator;
  auto stored = makeBaseManifest();
  auto current = makeBaseManifest();
  current.edition = 13;

  auto result = validator.validate(current, stored);
  CHECK(result.rebuildNeeded);
  CHECK(result.reason == RebuildReason::kEditionMismatch);
}

TEST_CASE("SencValidator update change -> rebuild", "[senc][validator]")
{
  SencValidator validator;
  auto stored = makeBaseManifest();
  auto current = makeBaseManifest();
  current.update = 4;

  auto result = validator.validate(current, stored);
  CHECK(result.rebuildNeeded);
  CHECK(result.reason == RebuildReason::kUpdateMismatch);
}

TEST_CASE("SencValidator size change -> rebuild", "[senc][validator]")
{
  SencValidator validator;
  auto stored = makeBaseManifest();
  auto current = makeBaseManifest();
  current.sourceSize = 999999;

  auto result = validator.validate(current, stored);
  CHECK(result.rebuildNeeded);
  CHECK(result.reason == RebuildReason::kSizeMismatch);
}

TEST_CASE("SencValidator timestamp newer -> rebuild", "[senc][validator]")
{
  SencValidator validator;
  auto stored = makeBaseManifest();
  auto current = makeBaseManifest();
  current.sourceTimestamp = stored.sourceTimestamp + 3600;

  auto result = validator.validate(current, stored);
  CHECK(result.rebuildNeeded);
  CHECK(result.reason == RebuildReason::kTimestampNewer);
}

TEST_CASE("SencValidator timestamp same -> no rebuild", "[senc][validator]")
{
  SencValidator validator;
  auto base = makeBaseManifest();
  auto result = validator.validate(base, base);
  CHECK_FALSE(result.rebuildNeeded);
}

TEST_CASE("SencValidator timestamp older -> no rebuild", "[senc][validator]")
{
  SencValidator validator;
  auto stored = makeBaseManifest();
  auto current = makeBaseManifest();
  current.sourceTimestamp = stored.sourceTimestamp - 100;

  auto result = validator.validate(current, stored);
  CHECK_FALSE(result.rebuildNeeded);
}

TEST_CASE("SencValidator hash change -> rebuild", "[senc][validator]")
{
  SencValidator validator;
  auto stored = makeBaseManifest();
  auto current = makeBaseManifest();
  current.sourceHash = 0xCAFEBABE;

  auto result = validator.validate(current, stored);
  CHECK(result.rebuildNeeded);
  CHECK(result.reason == RebuildReason::kHashMismatch);
}

TEST_CASE("SencValidator unknown size ignored", "[senc][validator]")
{
  SencValidator validator;
  auto stored = makeBaseManifest();
  stored.sourceSize = 0; // unknown
  auto current = makeBaseManifest();
  current.sourceSize = 999;

  auto result = validator.validate(current, stored);
  // Size check skipped when stored is 0.
  CHECK_FALSE(result.rebuildNeeded);
}

TEST_CASE("SencValidator unknown hash ignored", "[senc][validator]")
{
  SencValidator validator;
  auto stored = makeBaseManifest();
  stored.sourceHash = 0; // unknown
  auto current = makeBaseManifest();
  current.sourceHash = 0xAAAA;

  auto result = validator.validate(current, stored);
  CHECK_FALSE(result.rebuildNeeded);
}

TEST_CASE("SencValidator unknown timestamp ignored", "[senc][validator]")
{
  SencValidator validator;
  auto stored = makeBaseManifest();
  stored.sourceTimestamp = 0; // unknown
  auto current = makeBaseManifest();
  current.sourceTimestamp = 9999;

  auto result = validator.validate(current, stored);
  CHECK_FALSE(result.rebuildNeeded);
}

TEST_CASE("SencValidator manifest roundtrip via writer/reader", "[senc][validator]")
{
  // Write a SENC with an explicit manifest, read it back, validate.
  FeatureChartDataset ds;
  DatasetMeta meta;
  meta.name = "US5TX01M";
  meta.sourceType = chart_view_chart_source_s57;
  ds.setMeta(std::move(meta));

  SourceManifest writeManifest;
  writeManifest.name = "US5TX01M.000";
  writeManifest.sourceType = chart_view_chart_source_s57;
  writeManifest.sourceSize = 50000;
  writeManifest.sourceTimestamp = 1700000000;
  writeManifest.sourceHash = 0xABCDEF01;
  writeManifest.edition = 5;
  writeManifest.update = 2;

  SencWriter writer;
  writer.setSourceManifest(writeManifest);
  auto blob = writer.write(ds);

  SencReader reader;
  auto readResult = reader.read(blob);
  REQUIRE(readResult.ok);
  REQUIRE(readResult.manifest.has_value());

  const auto &readManifest = readResult.manifest.value();
  CHECK(readManifest.name == writeManifest.name);
  CHECK(readManifest.sourceType == writeManifest.sourceType);
  CHECK(readManifest.sourceSize == writeManifest.sourceSize);
  CHECK(readManifest.sourceTimestamp == writeManifest.sourceTimestamp);
  CHECK(readManifest.sourceHash == writeManifest.sourceHash);
  CHECK(readManifest.edition == writeManifest.edition);
  CHECK(readManifest.update == writeManifest.update);

  // Same manifest -> no rebuild.
  SencValidator validator;
  auto valResult = validator.validate(writeManifest, readManifest);
  CHECK_FALSE(valResult.rebuildNeeded);

  // Modified source -> rebuild.
  SourceManifest newer = writeManifest;
  newer.sourceTimestamp += 3600;
  valResult = validator.validate(newer, readManifest);
  CHECK(valResult.rebuildNeeded);
  CHECK(valResult.reason == RebuildReason::kTimestampNewer);
}
