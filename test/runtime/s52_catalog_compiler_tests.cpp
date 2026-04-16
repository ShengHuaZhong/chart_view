#include <catch2/catch_test_macros.hpp>

#include "portrayal/s52_source_catalog_compiler.hpp"

#include <algorithm>
#include <sstream>

namespace {

using chart_view::runtime::portrayal::S52CompiledCatalog;
using chart_view::runtime::portrayal::S52SourceCatalog;
using chart_view::runtime::portrayal::S52SourceCatalogCompiler;
using chart_view::runtime::portrayal::buildBuiltinS52SourceCatalog;

std::string catalogSignature(const S52CompiledCatalog &catalog)
{
  std::ostringstream stream;
  stream << "colors=" << catalog.colors.size() << ";";
  for(const auto &color : catalog.colors) {
    stream << color.token << ":"
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
           << static_cast<int>(row.geometryType) << ":" << row.displayCategory << ":";
    for(const auto &instruction : row.instructions) {
      stream << static_cast<int>(instruction.type) << "," << instruction.assetId << ","
             << instruction.styleKey << "|";
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
  REQUIRE(wreckRule->instructions.size() == 1);
  REQUIRE(wreckRule->instructions.front().assetId == "DANGER01");
  REQUIRE(wreckRule->instructions.front().styleKey == "point_danger");

  const auto depareRule = std::find_if(
    compiled.lookupRows.begin(),
    compiled.lookupRows.end(),
    [](const auto &row) { return row.objectAcronym == "DEPARE"; });
  REQUIRE(depareRule != compiled.lookupRows.end());
  REQUIRE(depareRule->ruleId == "s52_area_depare_area_depare01_area_depth");
  REQUIRE(depareRule->instructions.front().assetId == "DEPARE01");
  REQUIRE(depareRule->instructions.front().styleKey == "area_depth");
}
