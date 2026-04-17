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
    stream << point.assetId << ":" << point.colorToken << ":" << point.radius << ";";
  }

  stream << "lines=" << catalog.lineStyles.size() << ";";
  for(const auto &line : catalog.lineStyles) {
    stream << line.assetId << ":" << line.colorToken << ":" << line.thickness << ";";
  }

  stream << "areas=" << catalog.areaPatterns.size() << ";";
  for(const auto &area : catalog.areaPatterns) {
    stream << area.assetId << ":" << area.fillColorToken << ":" << area.outlineColorToken
           << ":" << area.holeFillColorToken << ":" << area.outlineThickness << ":"
           << static_cast<int>(area.fillAlpha) << ";";
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
  REQUIRE(compiled.lookupRows.size() > 3057);

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
  REQUIRE(depareOpenCpnRow->instructions.empty());
  REQUIRE_FALSE(depareOpenCpnRow->instructionFallback);

  const auto depareFallbackRow = std::find_if(
    compiled.lookupRows.begin(),
    compiled.lookupRows.end(),
    [](const auto &row) {
      return row.ruleId == "s52_area_depare_area_depare01_area_depth";
    });
  REQUIRE(depareFallbackRow != compiled.lookupRows.end());
  REQUIRE(depareFallbackRow->instructions.size() == 1);
  REQUIRE(depareFallbackRow->instructionFallback);
  REQUIRE(instructionType(depareFallbackRow->instructions.front())
          == chart_view::runtime::portrayal::S52InstructionType::kAreaPattern);
  REQUIRE(instructionAssetId(depareFallbackRow->instructions.front()) == "DEPARE01");
  REQUIRE(instructionStyleKey(depareFallbackRow->instructions.front()) == "area/depth");
}
