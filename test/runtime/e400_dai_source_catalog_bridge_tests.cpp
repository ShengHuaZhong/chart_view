#include <catch2/catch_test_macros.hpp>

#include "portrayal/e400_dai_source_catalog_bridge.hpp"
#include "portrayal/s52_source_catalog_compiler.hpp"

#include <algorithm>
#include <filesystem>

namespace {

std::filesystem::path daiPath()
{
  return std::filesystem::path(CHART_VIEW_PROJECT_SOURCE_DIR) / "docs" / "reference_local"
       / "PresLib_e4.0.0.dai";
}

} // namespace

TEST_CASE("e4.0.0 DAI bridge parses the local official source into a source catalog",
          "[portrayal][s52][dai][official]")
{
  using chart_view::runtime::chart_data::GeometryType;
  using chart_view::runtime::portrayal::E400DaiSourceCatalogBridge;
  using chart_view::runtime::portrayal::S52PaletteId;

  const auto result = E400DaiSourceCatalogBridge::parseFile(daiPath());

  INFO(result.error);
  REQUIRE(result.ok);

  const auto &catalog = result.catalog;
  REQUIRE(catalog.provenance.sourceFormat == "iho.preslib.dai");
  REQUIRE(catalog.provenance.sourceEdition == "04.0");
  REQUIRE(catalog.provenance.sourceRevision == "0");
  REQUIRE(catalog.provenance.sourceAgency == "IHO");
  REQUIRE(catalog.provenance.sourcePath == daiPath().string());
  REQUIRE(catalog.lookupRows.size() >= 1200);

  const auto nodtaDay = std::find_if(
    catalog.colors.begin(),
    catalog.colors.end(),
    [](const auto &color) { return color.tableName == "DAY" && color.token == "NODTA"; });
  REQUIRE(nodtaDay != catalog.colors.end());
  REQUIRE(nodtaDay->palette == S52PaletteId::kDay);

  const auto achareSymbol = std::find_if(
    catalog.pointSymbols.begin(),
    catalog.pointSymbols.end(),
    [](const auto &symbol) { return symbol.assetId == "ACHARE02"; });
  REQUIRE(achareSymbol != catalog.pointSymbols.end());
  REQUIRE(achareSymbol->sourceRcid == "SY01423");
  REQUIRE(achareSymbol->colorToken == "ACHMGD");
  REQUIRE(achareSymbol->definition == "V");
  REQUIRE(achareSymbol->vectorMetrics.width == 402);
  REQUIRE(achareSymbol->vectorMetrics.height == 503);
  REQUIRE(achareSymbol->vectorMetrics.pivot.valid);
  REQUIRE(achareSymbol->vectorMetrics.origin.valid);

  const auto arcSlnLine = std::find_if(
    catalog.lineStyles.begin(),
    catalog.lineStyles.end(),
    [](const auto &line) { return line.assetId == "ARCSLN01"; });
  REQUIRE(arcSlnLine != catalog.lineStyles.end());
  REQUIRE(arcSlnLine->sourceRcid == "LS01348");
  REQUIRE(arcSlnLine->colorToken == "ACHMGF");
  REQUIRE(arcSlnLine->hpgl.starts_with("SPA;SW1;PU200,800;PD800,800"));

  const auto airarePattern = std::find_if(
    catalog.areaPatterns.begin(),
    catalog.areaPatterns.end(),
    [](const auto &pattern) { return pattern.assetId == "AIRARE02"; });
  REQUIRE(airarePattern != catalog.areaPatterns.end());
  REQUIRE(airarePattern->sourceRcid == "PT01398");
  REQUIRE(airarePattern->definition == "V");
  REQUIRE(airarePattern->primaryColorToken == "ALANDF");
  REQUIRE(airarePattern->fillColorToken == "ALANDF");
  REQUIRE(airarePattern->outlineColorToken == "ALANDF");
  REQUIRE(airarePattern->holeFillColorToken == "NODTA");
  REQUIRE(airarePattern->hpgl.starts_with("SPA;SW1;PU623,980"));

  const auto achareLookup = std::find_if(
    catalog.lookupRows.begin(),
    catalog.lookupRows.end(),
    [](const auto &row) { return row.sourceLookupId == "LU00007"; });
  REQUIRE(achareLookup != catalog.lookupRows.end());
  REQUIRE(achareLookup->objectAcronym == "ACHARE");
  REQUIRE(achareLookup->geometryType == GeometryType::kArea);
  REQUIRE(achareLookup->tableName == "PLAIN_BOUNDARIES");
  REQUIRE(achareLookup->displayCategory == "Standard");
  REQUIRE(achareLookup->viewGroup == 26220U);
  REQUIRE(achareLookup->attributeCodes == std::vector<std::string>{"CATACH8"});
  REQUIRE(achareLookup->rawInstruction.find("SY(ACHARE02)") != std::string::npos);
  REQUIRE(achareLookup->rawInstruction.find("CS(RESTRN01)") != std::string::npos);
}

TEST_CASE("e4.0.0 DAI bridge output compiles into a usable official source catalog",
          "[portrayal][s52][dai][official][catalog]")
{
  using chart_view::runtime::portrayal::E400DaiSourceCatalogBridge;
  using chart_view::runtime::portrayal::S52SourceCatalogCompiler;
  using chart_view::runtime::portrayal::instructionAssetId;

  const auto parseResult = E400DaiSourceCatalogBridge::parseFile(daiPath());

  INFO(parseResult.error);
  REQUIRE(parseResult.ok);

  const auto compiled = S52SourceCatalogCompiler::compile(parseResult.catalog, "iho.preslib.e4_0_0");

  REQUIRE(compiled.catalogId == "iho.preslib.e4_0_0");
  REQUIRE(compiled.colors.size() >= 200);
  REQUIRE(compiled.pointSymbols.size() >= 500);
  REQUIRE(compiled.lineStyles.size() >= 40);
  REQUIRE(compiled.areaPatterns.size() >= 20);
  REQUIRE(compiled.lookupRows.size() == parseResult.catalog.lookupRows.size());
  REQUIRE(compiled.lookupRows.size() >= 1200);

  const auto achareSymbol = std::find_if(
    compiled.pointSymbols.begin(),
    compiled.pointSymbols.end(),
    [](const auto &symbol) { return symbol.assetId == "ACHARE02"; });
  REQUIRE(achareSymbol != compiled.pointSymbols.end());

  const auto arcSlnLine = std::find_if(
    compiled.lineStyles.begin(),
    compiled.lineStyles.end(),
    [](const auto &line) { return line.assetId == "ARCSLN01"; });
  REQUIRE(arcSlnLine != compiled.lineStyles.end());

  const auto airarePattern = std::find_if(
    compiled.areaPatterns.begin(),
    compiled.areaPatterns.end(),
    [](const auto &pattern) { return pattern.assetId == "AIRARE02"; });
  REQUIRE(airarePattern != compiled.areaPatterns.end());

  const auto achareLookup = std::find_if(
    compiled.lookupRows.begin(),
    compiled.lookupRows.end(),
    [](const auto &row) { return row.sourceLookupId == "LU00007"; });
  REQUIRE(achareLookup != compiled.lookupRows.end());
  REQUIRE(achareLookup->instructions.size() == 4);
  REQUIRE(instructionAssetId(achareLookup->instructions.front()) == "ACHARE02");
}
