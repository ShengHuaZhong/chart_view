#include <catch2/catch_test_macros.hpp>

#include "portrayal/opencpn_chartsymbols_parser.hpp"

#include <algorithm>
#include <filesystem>

namespace {

std::filesystem::path bundleRoot()
{
  return std::filesystem::path(CHART_VIEW_PROJECT_SOURCE_DIR) / "vendor" / "opencpn_s57data" / "Release_5.14.0"
       / "s57data";
}

} // namespace

TEST_CASE("OpenCPN chartsymbols parser loads the vendored bundle into a richer source catalog",
          "[portrayal][s52][opencpn]")
{
  using chart_view::runtime::portrayal::OpenCpnChartsymbolsParser;
  using chart_view::runtime::portrayal::OpenCpnS52ResourceBundle;
  using chart_view::runtime::portrayal::S52PaletteId;

  const OpenCpnS52ResourceBundle bundle{bundleRoot()};
  const auto result = OpenCpnChartsymbolsParser::parseBundle(bundle);

  INFO(result.error);
  REQUIRE(result.ok);

  const auto &catalog = result.catalog;
  REQUIRE(catalog.colors.size() == 315);
  REQUIRE(catalog.lookupRows.size() == 3057);
  REQUIRE(catalog.lineStyles.size() == 57);
  REQUIRE(catalog.areaPatterns.size() == 30);
  REQUIRE(catalog.pointSymbols.size() == 1093);

  const auto dayBrightColor = std::find_if(
    catalog.colors.begin(),
    catalog.colors.end(),
    [](const auto &color) {
      return color.tableName == "DAY_BRIGHT" && color.token == "NODTA" && color.graphicsFile == "rastersymbols-day.png";
    });
  REQUIRE(dayBrightColor != catalog.colors.end());
  REQUIRE(dayBrightColor->palette == S52PaletteId::kDay);
  REQUIRE(dayBrightColor->color[0] == 163U);
  REQUIRE(dayBrightColor->color[1] == 180U);
  REQUIRE(dayBrightColor->color[2] == 183U);

  const auto duskColor = std::find_if(
    catalog.colors.begin(),
    catalog.colors.end(),
    [](const auto &color) { return color.tableName == "DUSK" && color.token == "NODTA"; });
  REQUIRE(duskColor != catalog.colors.end());
  REQUIRE(duskColor->palette == S52PaletteId::kDusk);

  const auto nightColor = std::find_if(
    catalog.colors.begin(),
    catalog.colors.end(),
    [](const auto &color) { return color.tableName == "NIGHT" && color.token == "NODTA"; });
  REQUIRE(nightColor != catalog.colors.end());
  REQUIRE(nightColor->palette == S52PaletteId::kNight);
}

TEST_CASE("OpenCPN chartsymbols parser preserves representative lookup and asset metadata",
          "[portrayal][s52][opencpn]")
{
  using chart_view::runtime::chart_data::GeometryType;
  using chart_view::runtime::portrayal::OpenCpnChartsymbolsParser;
  using chart_view::runtime::portrayal::OpenCpnS52ResourceBundle;

  const OpenCpnS52ResourceBundle bundle{bundleRoot()};
  const auto result = OpenCpnChartsymbolsParser::parseBundle(bundle);

  INFO(result.error);
  REQUIRE(result.ok);

  const auto &catalog = result.catalog;

  const auto lookup = std::find_if(
    catalog.lookupRows.begin(),
    catalog.lookupRows.end(),
    [](const auto &row) { return row.sourceRcid == "32039"; });
  REQUIRE(lookup != catalog.lookupRows.end());
  REQUIRE(lookup->objectAcronym == "ACHBRT");
  REQUIRE(lookup->geometryType == GeometryType::kArea);
  REQUIRE(lookup->geometryTypeText == "Area");
  REQUIRE(lookup->displayPriorityText == "Line Symbol");
  REQUIRE(lookup->radarPriorityText == "Suppressed");
  REQUIRE(lookup->tableName == "Plain");
  REQUIRE(lookup->displayCategory == "Standard");
  REQUIRE(lookup->viewGroup == 26220U);
  REQUIRE(lookup->rawInstruction
          == "SY(ACHBRT07);TE('%s','OBJNAM',3,1,2,'15110',1,0,CHBLK,29);LS(DASH,2,CHMGF)");

  const auto attributedLookup = std::find_if(
    catalog.lookupRows.begin(),
    catalog.lookupRows.end(),
    [](const auto &row) { return row.sourceRcid == "32037"; });
  REQUIRE(attributedLookup != catalog.lookupRows.end());
  REQUIRE(attributedLookup->attributeCodes.size() == 1);
  REQUIRE(attributedLookup->attributeCodes.front() == "CATACH8");

  const auto symbol = std::find_if(
    catalog.pointSymbols.begin(),
    catalog.pointSymbols.end(),
    [](const auto &entry) { return entry.assetId == "ACHARE02"; });
  REQUIRE(symbol != catalog.pointSymbols.end());
  REQUIRE(symbol->sourceRcid == "2035");
  REQUIRE(symbol->description
          == "anchorage area as a point at small scale, or anchor points of mooring trot at large scale");
  REQUIRE(symbol->colorToken == "ACHMGD");
  REQUIRE(symbol->definition == "V");
  REQUIRE(symbol->bitmapMetrics.width == 13);
  REQUIRE(symbol->bitmapMetrics.height == 16);
  REQUIRE(symbol->bitmapMetrics.pivot.valid);
  REQUIRE(symbol->bitmapMetrics.pivot.x == 6);
  REQUIRE(symbol->bitmapMetrics.pivot.y == 8);
  REQUIRE(symbol->bitmapMetrics.origin.valid);
  REQUIRE(symbol->bitmapMetrics.origin.x == 0);
  REQUIRE(symbol->bitmapMetrics.origin.y == 0);
  REQUIRE(symbol->vectorMetrics.width == 402);
  REQUIRE(symbol->vectorMetrics.height == 503);
  REQUIRE(symbol->vectorMetrics.pivot.valid);
  REQUIRE(symbol->vectorMetrics.pivot.x == 1267);
  REQUIRE(symbol->vectorMetrics.pivot.y == 1052);
  REQUIRE(symbol->vectorMetrics.origin.valid);
  REQUIRE(symbol->vectorMetrics.origin.x == 1061);
  REQUIRE(symbol->vectorMetrics.origin.y == 789);

  const auto lineStyle = std::find_if(
    catalog.lineStyles.begin(),
    catalog.lineStyles.end(),
    [](const auto &entry) { return entry.assetId == "ACHARE51"; });
  REQUIRE(lineStyle != catalog.lineStyles.end());
  REQUIRE(lineStyle->sourceRcid == "3346");
  REQUIRE(lineStyle->colorToken == "ACHMGD");
  REQUIRE(lineStyle->vectorMetrics.width == 3030);
  REQUIRE(lineStyle->vectorMetrics.height == 503);
  REQUIRE(lineStyle->vectorMetrics.pivot.valid);
  REQUIRE(lineStyle->vectorMetrics.pivot.x == 108);
  REQUIRE(lineStyle->vectorMetrics.pivot.y == 820);
  REQUIRE(lineStyle->vectorMetrics.origin.valid);
  REQUIRE(lineStyle->vectorMetrics.origin.x == 306);
  REQUIRE(lineStyle->vectorMetrics.origin.y == 568);
  REQUIRE(lineStyle->hpgl.starts_with("SPA;SW1;PU1429,568"));

  const auto pattern = std::find_if(
    catalog.areaPatterns.begin(),
    catalog.areaPatterns.end(),
    [](const auto &entry) { return entry.assetId == "AIRARE02"; });
  REQUIRE(pattern != catalog.areaPatterns.end());
  REQUIRE(pattern->sourceRcid == "2000");
  REQUIRE(pattern->definition == "V");
  REQUIRE(pattern->fillType == "S");
  REQUIRE(pattern->spacing == "C");
  REQUIRE(pattern->primaryColorToken == "ALANDF");
  REQUIRE(pattern->vectorMetrics.width == 618);
  REQUIRE(pattern->vectorMetrics.height == 528);
  REQUIRE(pattern->vectorMetrics.pivot.valid);
  REQUIRE(pattern->vectorMetrics.origin.valid);
  REQUIRE(pattern->hpgl.starts_with("SPA;SW1;PU623,980"));
}
