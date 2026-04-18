#include <catch2/catch_test_macros.hpp>

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "support/s52_resource_snapshot_inventory.hpp"

#include <cstdlib>
#include <array>
#include <filesystem>
#include <set>
#include <string_view>

namespace {

std::filesystem::path referencePath()
{
  return std::filesystem::path(CHART_VIEW_PROJECT_SOURCE_DIR) / "tests" / "data" / "reference"
       / "phase6b_s52_resource_snapshot_inventory.reference.json";
}

bool shouldWriteReference()
{
  char *value = nullptr;
  std::size_t valueLength = 0;
  if(_dupenv_s(&value, &valueLength, "CHART_VIEW_WRITE_REFERENCE") != 0 || value == nullptr) {
    return false;
  }

  const std::string_view view{value, valueLength > 0 ? valueLength - 1 : 0};
  const bool shouldWrite = view == "1";
  std::free(value);
  return shouldWrite;
}

QByteArray loadExpectedReference(const std::filesystem::path &path)
{
  QFile file(QString::fromStdWString(path.wstring()));
  if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return {};
  }

  return file.readAll();
}

} // namespace

TEST_CASE("S52 resource snapshot coverage inventory is deterministic and matches the committed baseline",
          "[portrayal][s52][inventory][phase6b]")
{
  const auto projectSourceDir = std::filesystem::path(CHART_VIEW_PROJECT_SOURCE_DIR);
  const auto inventoryA = chart_view::test_support::buildS52ResourceSnapshotInventory(projectSourceDir);
  const auto inventoryB = chart_view::test_support::buildS52ResourceSnapshotInventory(projectSourceDir);

  REQUIRE(inventoryA.lookupRowsTotal == 3057);
  REQUIRE(inventoryA.lookupRowsTotal == inventoryB.lookupRowsTotal);
  REQUIRE(inventoryA.supportedRows == inventoryB.supportedRows);
  REQUIRE(inventoryA.partialRows == inventoryB.partialRows);
  REQUIRE(inventoryA.unsupportedRows == inventoryB.unsupportedRows);
  REQUIRE(inventoryA.document.toJson(QJsonDocument::Compact) == inventoryB.document.toJson(QJsonDocument::Compact));

  const auto path = referencePath();
  if(shouldWriteReference()) {
    QFile file(QString::fromStdWString(path.wstring()));
    REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate));
    file.write(inventoryA.document.toJson(QJsonDocument::Indented));
    file.close();
  }

  const auto expected = loadExpectedReference(path);
  REQUIRE_FALSE(expected.isEmpty());
  REQUIRE(expected == inventoryA.document.toJson(QJsonDocument::Indented));
}

TEST_CASE("S52 resource snapshot inventory no longer reports compiler-missing lookup rows for wave1 families",
          "[portrayal][s52][inventory][phase6c][wave1]")
{
  const auto projectSourceDir = std::filesystem::path(CHART_VIEW_PROJECT_SOURCE_DIR);
  const auto inventory = chart_view::test_support::buildS52ResourceSnapshotInventory(projectSourceDir);
  const auto root = inventory.document.object();
  const auto degradedRows = root.value("degradedRows").toArray();

  const std::set<QString> wave1Objects = {
    "NOTMRK",
    "TERMNL",
    "BOYWTW",
    "BOYLAT",
    "TOPMAR",
    "BCNLAT",
    "HRBFAC",
    "POSITN",
    "OBSTRN",
    "RDOCAL",
    "VEHTRF",
    "RESARE"};

  for(const auto &rowValue : degradedRows) {
    const auto row = rowValue.toObject();
    const auto objectAcronym = row.value("objectAcronym").toString().trimmed().toUpper();
    if(!wave1Objects.contains(objectAcronym)) {
      continue;
    }

    const auto reasons = row.value("reasons").toArray();
    for(const auto &reasonValue : reasons) {
      REQUIRE(reasonValue.toString() != "compiler_missing_lookup_row");
    }
  }
}

TEST_CASE("S52 resource snapshot inventory canonicalizes wave1 point-asset reasons",
          "[portrayal][s52][inventory][phase6c][wave1][assets]")
{
  const auto projectSourceDir = std::filesystem::path(CHART_VIEW_PROJECT_SOURCE_DIR);
  const auto inventory = chart_view::test_support::buildS52ResourceSnapshotInventory(projectSourceDir);
  const auto root = inventory.document.object();
  const auto degradedRows = root.value("degradedRows").toArray();

  bool sawRdocal02 = false;
  bool sawRdocal03 = false;
  bool sawTopmar90 = false;
  bool sawTopmar93 = false;

  for(const auto &rowValue : degradedRows) {
    const auto row = rowValue.toObject();
    const auto reasons = row.value("reasons").toArray();
    const auto pointAssets = row.value("pointAssets").toArray();

    for(const auto &assetValue : pointAssets) {
      const auto assetId = assetValue.toString();
      if(assetId == "RDOCAL02") {
        sawRdocal02 = true;
      } else if(assetId == "RDOCAL03") {
        sawRdocal03 = true;
      } else if(assetId == "TOPMAR90") {
        sawTopmar90 = true;
      } else if(assetId == "TOPMAR93") {
        sawTopmar93 = true;
      }
    }

    for(const auto &reasonValue : reasons) {
      const auto reason = reasonValue.toString();
      REQUIRE(reason != "compiler_missing_point_asset:rdocal02");
      REQUIRE(reason != "compiler_missing_point_asset:rdocal03");
      REQUIRE(reason != "compiler_missing_point_asset:TOPMAR90");
      REQUIRE(reason != "compiler_missing_point_asset:TOPMAR93");
    }
  }

  REQUIRE(sawRdocal02);
  REQUIRE(sawRdocal03);
  REQUIRE(sawTopmar90);
  REQUIRE(sawTopmar93);
}

TEST_CASE("S52 resource snapshot inventory clears task 107 manual-overlay asset reasons",
          "[portrayal][s52][inventory][phase6d][task107]")
{
  const auto projectSourceDir = std::filesystem::path(CHART_VIEW_PROJECT_SOURCE_DIR);
  const auto inventory = chart_view::test_support::buildS52ResourceSnapshotInventory(projectSourceDir);
  const auto root = inventory.document.object();
  const auto degradedRows = root.value("degradedRows").toArray();

  bool sawArcSln = false;
  bool sawNewObj = false;

  for(const auto &rowValue : degradedRows) {
    const auto row = rowValue.toObject();
    const auto objectAcronym = row.value("objectAcronym").toString().trimmed().toUpper();
    const auto reasons = row.value("reasons").toArray();

    if(objectAcronym == "ARCSLN") {
      sawArcSln = true;
    }
    if(objectAcronym == "NEWOBJ") {
      sawNewObj = true;
    }

    for(const auto &reasonValue : reasons) {
      const auto reason = reasonValue.toString();
      REQUIRE(reason != "compiler_missing_line_asset:ARCSLN01");
      REQUIRE(reason != "compiler_missing_line_asset:NEWOBJ01");
      REQUIRE(reason != "compiler_missing_point_asset:NEWOBJ01");
    }
  }

  REQUIRE(sawArcSln);
  REQUIRE(sawNewObj);
}
