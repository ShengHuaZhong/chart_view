#include <catch2/catch_test_macros.hpp>

#include <QFile>
#include <QJsonDocument>

#include "support/s52_resource_snapshot_inventory.hpp"

#include <cstdlib>
#include <filesystem>
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
