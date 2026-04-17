#include <catch2/catch_test_macros.hpp>

#include "portrayal/opencpn_s52_resource_bundle.hpp"
#include "portrayal/s52_source_catalog_compiler.hpp"

#include <algorithm>
#include <filesystem>
#include <sstream>

namespace {

using chart_view::runtime::portrayal::S52CompiledCatalog;
using chart_view::runtime::portrayal::OpenCpnS52ResourceBundle;
using chart_view::runtime::portrayal::S52SourceCatalog;
using chart_view::runtime::portrayal::S52SourceCatalogCompiler;
using chart_view::runtime::portrayal::buildBuiltinS52SourceCatalog;
using chart_view::runtime::portrayal::instructionAssetId;
using chart_view::runtime::portrayal::instructionConditionalOpcode;
using chart_view::runtime::portrayal::instructionStyleKey;
using chart_view::runtime::portrayal::instructionType;

std::filesystem::path bundleRoot()
{
  const auto sourceFile = std::filesystem::path(__FILE__);
  return sourceFile.parent_path().parent_path().parent_path() / "vendor" / "opencpn_s57data"
       / "Release_5.14.0" / "s57data";
}

std::string catalogSignature(const S52CompiledCatalog &catalog)
{
  std::ostringstream stream;
  stream << "catalogId=" << catalog.catalogId << ";";
  stream << "colors=" << catalog.colors.size() << ";";
  for(const auto &color : catalog.colors) {
    stream << color.token << ":" << static_cast<int>(color.palette) << ":" << color.tableName << ":"
           << static_cast<int>(color.color[0]) << ","
           << static_cast<int>(color.color[1]) << ","
           << static_cast<int>(color.color[2]) << ","
           << static_cast<int>(color.color[3]) << ";";
  }

  stream << "points=" << catalog.pointSymbols.size() << ";";
  for(const auto &point : catalog.pointSymbols) {
    stream << point.assetId << ":" << point.colorToken << ":" << point.radius << ":"
           << point.sourceRcid << ":" << point.bitmapMetrics.width << ":" << point.bitmapMetrics.height
           << ":" << point.bitmapMetrics.pivot.x << ":" << point.bitmapMetrics.pivot.y << ":"
           << point.bitmapMetrics.origin.x << ":" << point.bitmapMetrics.origin.y << ":"
           << point.vectorMetrics.width << ":" << point.vectorMetrics.height << ":"
           << point.vectorMetrics.pivot.x << ":" << point.vectorMetrics.pivot.y << ":"
           << point.vectorMetrics.origin.x << ":" << point.vectorMetrics.origin.y << ":"
           << point.preferBitmap << ";";
  }

  stream << "lines=" << catalog.lineStyles.size() << ";";
  for(const auto &line : catalog.lineStyles) {
    stream << line.assetId << ":" << line.colorToken << ":" << line.thickness << ":" << line.sourceRcid
           << ":" << line.vectorMetrics.width << ":" << line.vectorMetrics.height << ":"
           << line.vectorMetrics.pivot.x << ":" << line.vectorMetrics.pivot.y << ":"
           << line.vectorMetrics.origin.x << ":" << line.vectorMetrics.origin.y << ":" << line.hpgl
           << ";";
  }

  stream << "areas=" << catalog.areaPatterns.size() << ";";
  for(const auto &area : catalog.areaPatterns) {
    stream << area.assetId << ":" << area.fillColorToken << ":" << area.outlineColorToken
           << ":" << area.holeFillColorToken << ":" << area.outlineThickness << ":"
           << static_cast<int>(area.fillAlpha) << ":" << area.sourceRcid << ":" << area.fillType
           << ":" << area.spacingToken << ":" << area.primaryColorToken << ":"
           << area.bitmapMetrics.width << ":" << area.bitmapMetrics.height << ":"
           << area.vectorMetrics.width << ":" << area.vectorMetrics.height << ":" << area.hpgl << ";";
  }

  stream << "rows=" << catalog.lookupRows.size() << ";";
  for(const auto &row : catalog.lookupRows) {
    stream << row.ruleId << ":" << row.objectAcronym << ":"
           << static_cast<int>(row.geometryType) << ":" << row.displayCategory << ":"
           << row.displayPriority << ":" << row.viewGroup << ":" << row.sourceLookupId << ":"
           << row.sourceRcid << ":" << row.tableName << ":" << row.radarPriorityText << ":"
           << row.rawInstruction << ":" << row.instructionFallback << ":";
    for(const auto &instruction : row.instructions) {
      stream << static_cast<int>(instructionType(instruction)) << ","
             << instructionAssetId(instruction) << ","
             << instructionStyleKey(instruction) << "|";
    }
    stream << ";";
  }

  return stream.str();
}

} // namespace

TEST_CASE("S52SourceCatalogCompiler produces deterministic output from shuffled source data",
          "[portrayal][s52][catalog]")
{
  auto sourceA = buildBuiltinS52SourceCatalog();
  auto sourceB = sourceA;

  std::reverse(sourceB.colors.begin(), sourceB.colors.end());
  std::reverse(sourceB.pointSymbols.begin(), sourceB.pointSymbols.end());
  std::reverse(sourceB.lineStyles.begin(), sourceB.lineStyles.end());
  std::reverse(sourceB.areaPatterns.begin(), sourceB.areaPatterns.end());
  std::reverse(sourceB.lookupRows.begin(), sourceB.lookupRows.end());

  const auto compiledA = S52SourceCatalogCompiler::compile(sourceA);
  const auto compiledB = S52SourceCatalogCompiler::compile(sourceB);

  REQUIRE(catalogSignature(compiledA) == catalogSignature(compiledB));
}

TEST_CASE("S52SourceCatalogCompiler exposes sane baseline lookup rows and stable rule ids",
          "[portrayal][s52][catalog]")
{
  const auto compiled = S52SourceCatalogCompiler::compileBuiltin();

  REQUIRE(compiled.colors.size() == 12);
  REQUIRE(compiled.pointSymbols.size() == 7);
  REQUIRE(compiled.lineStyles.size() == 3);
  REQUIRE(compiled.areaPatterns.size() == 3);
  REQUIRE(compiled.lookupRows.size() == 22);

  const auto wreckRule = std::find_if(
    compiled.lookupRows.begin(),
    compiled.lookupRows.end(),
    [](const auto &row) { return row.objectAcronym == "WRECKS"; });
  REQUIRE(wreckRule != compiled.lookupRows.end());
  REQUIRE(wreckRule->ruleId == "s52_point_wrecks_point_danger01_point_danger");
  REQUIRE(wreckRule->displayPriority == 300);
  REQUIRE(wreckRule->viewGroup == 33010U);
  REQUIRE(wreckRule->instructions.size() == 1);
  REQUIRE(instructionAssetId(wreckRule->instructions.front()) == "DANGER01");
  REQUIRE(instructionStyleKey(wreckRule->instructions.front()) == "point/danger");

  const auto depareRule = std::find_if(
    compiled.lookupRows.begin(),
    compiled.lookupRows.end(),
    [](const auto &row) { return row.objectAcronym == "DEPARE"; });
  REQUIRE(depareRule != compiled.lookupRows.end());
  REQUIRE(depareRule->ruleId == "s52_area_depare_area_depare01_area_depth");
  REQUIRE(depareRule->displayPriority == 100);
  REQUIRE(depareRule->viewGroup == 13010U);
  REQUIRE(instructionAssetId(depareRule->instructions.front()) == "DEPARE01");
  REQUIRE(instructionStyleKey(depareRule->instructions.front()) == "area/depth");
}

TEST_CASE("S52SourceCatalogCompiler compiles the vendored OpenCPN bundle into the preferred deterministic catalog",
          "[portrayal][s52][catalog][opencpn]")
{
  std::string error;
  const auto compiled = S52SourceCatalogCompiler::compileOpenCpnBundle({bundleRoot()}, &error);

  INFO(error);
  REQUIRE(error.empty());
  REQUIRE(compiled.catalogId == "opencpn.release_5_14_0");
  REQUIRE(compiled.colors.size() == 315);
  REQUIRE(compiled.pointSymbols.size() >= 1091);
  REQUIRE(compiled.lineStyles.size() >= 57);
  REQUIRE(compiled.areaPatterns.size() >= 3);
  REQUIRE(compiled.lookupRows.size() == 3057);

  const auto depareOpenCpnRow = std::find_if(
    compiled.lookupRows.begin(),
    compiled.lookupRows.end(),
    [](const auto &row) {
      return row.objectAcronym == "DEPARE" && row.sourceRcid == "32075";
    });
  REQUIRE(depareOpenCpnRow != compiled.lookupRows.end());
  REQUIRE(depareOpenCpnRow->ruleId == "s52_area_depare_plain_rcid_32075");
  REQUIRE(depareOpenCpnRow->rawInstruction == "AC(NODTA);AP(PRTSUR01);LS(SOLD,2,CHGRD)");
  REQUIRE(depareOpenCpnRow->tableName == "Plain");
  REQUIRE(depareOpenCpnRow->instructions.size() == 3);
  REQUIRE_FALSE(depareOpenCpnRow->instructionFallback);
  REQUIRE(instructionType(depareOpenCpnRow->instructions[0])
          == chart_view::runtime::portrayal::S52InstructionType::kAreaColor);
  REQUIRE(instructionAssetId(depareOpenCpnRow->instructions[0]) == "NODTA");
  REQUIRE(instructionType(depareOpenCpnRow->instructions[1])
          == chart_view::runtime::portrayal::S52InstructionType::kAreaPattern);
  REQUIRE(instructionAssetId(depareOpenCpnRow->instructions[1]) == "PRTSUR01");
  REQUIRE(instructionType(depareOpenCpnRow->instructions[2])
          == chart_view::runtime::portrayal::S52InstructionType::kLineStyle);
  REQUIRE(instructionAssetId(depareOpenCpnRow->instructions[2]) == "LS_SOLD_2_CHGRD");

  const auto achareRow = std::find_if(
    compiled.lookupRows.begin(),
    compiled.lookupRows.end(),
    [](const auto &row) {
      return row.objectAcronym == "ACHARE" && row.sourceRcid == "32037";
    });
  REQUIRE(achareRow != compiled.lookupRows.end());
  REQUIRE(achareRow->instructions.size() == 3);
  REQUIRE(achareRow->attributeCodes == std::vector<std::string>{"CATACH8"});
  REQUIRE(instructionType(achareRow->instructions[0])
          == chart_view::runtime::portrayal::S52InstructionType::kPointSymbol);
  REQUIRE(instructionAssetId(achareRow->instructions[0]) == "ACHARE02");
  REQUIRE(instructionType(achareRow->instructions[1])
          == chart_view::runtime::portrayal::S52InstructionType::kLineStyle);
  REQUIRE(instructionAssetId(achareRow->instructions[1]) == "LS_DASH_2_CHMGF");
  REQUIRE(instructionType(achareRow->instructions[2])
          == chart_view::runtime::portrayal::S52InstructionType::kConditional);
  const auto *achareConditional =
    std::get_if<chart_view::runtime::portrayal::S52ConditionalInstruction>(&achareRow->instructions[2]);
  REQUIRE(achareConditional != nullptr);
  REQUIRE(achareConditional->conditionId == "RESTRN01");
  REQUIRE(
    instructionConditionalOpcode(achareRow->instructions[2])
    == chart_view::runtime::portrayal::S52ConditionalOpcode::kRestrn01);

  const auto depareFallbackRow = std::find_if(
    compiled.lookupRows.begin(),
    compiled.lookupRows.end(),
    [](const auto &row) {
      return row.ruleId == "s52_area_depare_area_depare01_area_depth";
    });
  REQUIRE(depareFallbackRow == compiled.lookupRows.end());

  const auto achareSymbol = std::find_if(
    compiled.pointSymbols.begin(),
    compiled.pointSymbols.end(),
    [](const auto &symbol) { return symbol.assetId == "ACHARE02"; });
  REQUIRE(achareSymbol != compiled.pointSymbols.end());
  REQUIRE(achareSymbol->sourceRcid == "2035");
  REQUIRE(achareSymbol->vectorMetrics.pivot.valid);
  REQUIRE(achareSymbol->vectorMetrics.origin.valid);
  REQUIRE(achareSymbol->vectorMetrics.pivot.x == 1267);
  REQUIRE(achareSymbol->vectorMetrics.origin.x == 1061);

  const auto achareLine = std::find_if(
    compiled.lineStyles.begin(),
    compiled.lineStyles.end(),
    [](const auto &line) { return line.assetId == "ACHARE51"; });
  REQUIRE(achareLine != compiled.lineStyles.end());
  REQUIRE(achareLine->sourceRcid == "3346");
  REQUIRE(achareLine->vectorMetrics.pivot.valid);
  REQUIRE(achareLine->vectorMetrics.origin.valid);
  REQUIRE(achareLine->hpgl.starts_with("SPA;SW1;PU1429,568"));

  const auto syntheticDashLine = std::find_if(
    compiled.lineStyles.begin(),
    compiled.lineStyles.end(),
    [](const auto &line) { return line.assetId == "LS_DASH_2_CHMGF"; });
  REQUIRE(syntheticDashLine != compiled.lineStyles.end());
  REQUIRE(syntheticDashLine->colorToken == "CHMGF");
  REQUIRE(syntheticDashLine->thickness == 2);
  REQUIRE(syntheticDashLine->sourceRcid.empty());
  REQUIRE(syntheticDashLine->description == "synthetic line style compiled from LS(...)");

  const auto prtsurPattern = std::find_if(
    compiled.areaPatterns.begin(),
    compiled.areaPatterns.end(),
    [](const auto &pattern) { return pattern.assetId == "PRTSUR01"; });
  REQUIRE(prtsurPattern != compiled.areaPatterns.end());
  REQUIRE(prtsurPattern->fillType == "S");
  REQUIRE(prtsurPattern->spacingToken == "C");
  REQUIRE(prtsurPattern->primaryColorToken == "ACHGRD");
  REQUIRE(prtsurPattern->holeFillColorToken == "NODTA");
}
