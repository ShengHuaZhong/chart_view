#include <catch2/catch_test_macros.hpp>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "support/e400_official_path_row_coverage_inventory.hpp"

#include <cstdlib>
#include <filesystem>
#include <string_view>

namespace {

std::filesystem::path referencePath()
{
  return std::filesystem::path(CHART_VIEW_PROJECT_SOURCE_DIR) / "tests" / "data" / "reference"
       / "phase6e_official_path_row_coverage.reference.json";
}

std::filesystem::path officialDaiPath()
{
  return std::filesystem::path(CHART_VIEW_PROJECT_SOURCE_DIR) / "docs" / "reference_local"
       / "PresLib_e4.0.0.dai";
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

TEST_CASE("official e4.0.0 path row coverage baseline is deterministic and matches the committed baseline",
          "[portrayal][s52][official][e400][inventory]")
{
  if(!std::filesystem::exists(officialDaiPath())) {
    SKIP("local official e4.0.0 DAI reference is missing");
  }

  const auto projectSourceDir = std::filesystem::path(CHART_VIEW_PROJECT_SOURCE_DIR);
  const auto inventoryA = chart_view::test_support::buildE400OfficialPathRowCoverageInventory(projectSourceDir);
  const auto inventoryB = chart_view::test_support::buildE400OfficialPathRowCoverageInventory(projectSourceDir);

  REQUIRE(inventoryA.document.object().value("ok").toBool());
  REQUIRE(inventoryA.lookupRowsTotal >= 1200);
  REQUIRE(inventoryA.lookupRowsTotal == inventoryB.lookupRowsTotal);
  REQUIRE(inventoryA.supportedRows == inventoryB.supportedRows);
  REQUIRE(inventoryA.partialRows == inventoryB.partialRows);
  REQUIRE(inventoryA.unsupportedRows == inventoryB.unsupportedRows);
  REQUIRE(inventoryA.document.toJson(QJsonDocument::Compact) == inventoryB.document.toJson(QJsonDocument::Compact));

  const auto root = inventoryA.document.object();
  const auto officialSource = root.value("officialSource").toObject();
  REQUIRE(officialSource.value("catalogId").toString() == "iho.preslib.e4_0_0");
  REQUIRE(officialSource.value("sourceFormat").toString() == "iho.preslib.dai");
  REQUIRE(officialSource.value("sourceEdition").toString() == "04.0");

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
