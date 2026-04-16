#include "s52_source_catalog_compiler.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>
#include <tuple>
#include <utility>

namespace chart_view::runtime::portrayal {

namespace {

std::string normalizeToken(std::string_view value)
{
  std::string normalized;
  normalized.reserve(value.size());
  for(const auto ch : value) {
    if(std::isalnum(static_cast<unsigned char>(ch)) != 0) {
      normalized.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    } else if(ch == '_' || ch == '-' || ch == '/' || ch == '.') {
      normalized.push_back('_');
    }
  }
  return normalized;
}

std::string normalizeRuleId(std::string_view value)
{
  std::string normalized;
  normalized.reserve(value.size());
  for(const auto ch : value) {
    if(std::isalnum(static_cast<unsigned char>(ch)) != 0) {
      normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    } else if(ch == '_' || ch == '-' || ch == '/' || ch == '.') {
      normalized.push_back('_');
    }
  }
  return normalized;
}

std::string geometryTypeToken(chart_data::GeometryType geometryType)
{
  switch(geometryType) {
  case chart_data::GeometryType::kPoint:
    return "point";
  case chart_data::GeometryType::kLine:
    return "line";
  case chart_data::GeometryType::kArea:
    return "area";
  }

  return "unknown";
}

std::string instructionTypeToken(S52CompiledInstructionType type)
{
  switch(type) {
  case S52CompiledInstructionType::kPointSymbol:
    return "point";
  case S52CompiledInstructionType::kLineStyle:
    return "line";
  case S52CompiledInstructionType::kAreaPattern:
    return "area";
  case S52CompiledInstructionType::kTextLabel:
    return "text";
  }

  return "unknown";
}

std::string makeStableRuleId(const S52SourceLookupRow &row)
{
  if(!row.ruleId.empty()) {
    return normalizeRuleId(row.ruleId);
  }

  std::string stableId = "s52_";
  stableId += geometryTypeToken(row.geometryType);
  stableId += "_";
  stableId += normalizeRuleId(row.objectAcronym);
  for(const auto &instruction : row.instructions) {
    stableId += "_";
    stableId += instructionTypeToken(instruction.type);
    stableId += "_";
    stableId += normalizeRuleId(instruction.assetId);
    if(!instruction.styleKey.empty()) {
      stableId += "_";
      stableId += normalizeRuleId(instruction.styleKey);
    }
  }
  return stableId;
}

SurfaceColor makeColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255U)
{
  return {r, g, b, a};
}

} // namespace

S52SourceCatalog buildBuiltinS52SourceCatalog()
{
  S52SourceCatalog catalog;

  catalog.colors = {
    {S52PaletteId::kDay, "CHBLK", makeColor(24U, 38U, 55U)},
    {S52PaletteId::kDay, "CHBRN", makeColor(110U, 96U, 52U)},
    {S52PaletteId::kDay, "CHGRD", makeColor(70U, 70U, 70U)},
    {S52PaletteId::kDay, "CHGRN", makeColor(24U, 116U, 86U)},
    {S52PaletteId::kDay, "CHRED", makeColor(160U, 58U, 58U)},
    {S52PaletteId::kDay, "CHYLW", makeColor(220U, 176U, 32U)},
    {S52PaletteId::kDay, "DEPDW", makeColor(162U, 201U, 229U)},
    {S52PaletteId::kDay, "DEPSC", makeColor(44U, 91U, 134U)},
    {S52PaletteId::kDay, "DNGHL", makeColor(210U, 92U, 28U)},
    {S52PaletteId::kDay, "LANDF", makeColor(196U, 190U, 137U)},
    {S52PaletteId::kDay, "NODTA", makeColor(230U, 230U, 217U)},
    {S52PaletteId::kDay, "RESDR", makeColor(229U, 196U, 196U)},
  };

  catalog.pointSymbols = {
    {"SOUNDG01", "CHBLK", 4},
    {"BOYSPP01", "CHYLW", 4},
    {"BOYSPP02", "CHYLW", 3},
    {"BCNSPP01", "CHBRN", 4},
    {"BCNSPP02", "CHBRN", 3},
    {"DANGER01", "DNGHL", 4},
    {"LNDMRK01", "CHGRD", 4},
  };

  catalog.lineStyles = {
    {"DEPCN01", "CHBLK", 2},
    {"COALNE01", "CHBLK", 2},
    {"FAIRWY01", "CHGRN", 2},
  };

  catalog.areaPatterns = {
    {"DEPARE01", "DEPDW", "DEPSC", "NODTA", 1, 204U},
    {"LNDARE01", "LANDF", "CHBRN", "NODTA", 1, 255U},
    {"RESARE01", "RESDR", "CHRED", "NODTA", 1, 220U},
  };

  const auto addLookupRow = [&](std::string_view objectAcronym,
                                chart_data::GeometryType geometryType,
                                S52CompiledInstructionType instructionType,
                                std::string_view assetId,
                                std::string_view styleKey) {
    S52SourceLookupRow row;
    row.objectAcronym = std::string(objectAcronym);
    row.geometryType = geometryType;
    row.instructions.push_back(
      {instructionType, std::string(assetId), std::string(styleKey)});
    catalog.lookupRows.push_back(std::move(row));
  };

  addLookupRow("SOUNDG", chart_data::GeometryType::kPoint, S52CompiledInstructionType::kPointSymbol, "SOUNDG01", "point/sounding");
  addLookupRow("BOYSPP", chart_data::GeometryType::kPoint, S52CompiledInstructionType::kPointSymbol, "BOYSPP01", "point/buoy");
  addLookupRow("BOYLAT", chart_data::GeometryType::kPoint, S52CompiledInstructionType::kPointSymbol, "BOYSPP01", "point/buoy");
  addLookupRow("BOYSAW", chart_data::GeometryType::kPoint, S52CompiledInstructionType::kPointSymbol, "BOYSPP01", "point/buoy");
  addLookupRow("BCNSPP", chart_data::GeometryType::kPoint, S52CompiledInstructionType::kPointSymbol, "BCNSPP01", "point/beacon");
  addLookupRow("BCNLAT", chart_data::GeometryType::kPoint, S52CompiledInstructionType::kPointSymbol, "BCNSPP01", "point/beacon");
  addLookupRow("BCNSAW", chart_data::GeometryType::kPoint, S52CompiledInstructionType::kPointSymbol, "BCNSPP01", "point/beacon");
  addLookupRow("WRECKS", chart_data::GeometryType::kPoint, S52CompiledInstructionType::kPointSymbol, "DANGER01", "point/danger");
  addLookupRow("UWTROC", chart_data::GeometryType::kPoint, S52CompiledInstructionType::kPointSymbol, "DANGER01", "point/danger");
  addLookupRow("LIGHTS", chart_data::GeometryType::kPoint, S52CompiledInstructionType::kPointSymbol, "LNDMRK01", "point/landmark");
  addLookupRow("LNDMRK", chart_data::GeometryType::kPoint, S52CompiledInstructionType::kPointSymbol, "LNDMRK01", "point/landmark");
  addLookupRow("PILPNT", chart_data::GeometryType::kPoint, S52CompiledInstructionType::kPointSymbol, "LNDMRK01", "point/landmark");

  addLookupRow("DEPCNT", chart_data::GeometryType::kLine, S52CompiledInstructionType::kLineStyle, "DEPCN01", "line/depth_contour");
  addLookupRow("COALNE", chart_data::GeometryType::kLine, S52CompiledInstructionType::kLineStyle, "COALNE01", "line/coastline");
  addLookupRow("FAIRWY", chart_data::GeometryType::kLine, S52CompiledInstructionType::kLineStyle, "FAIRWY01", "line/channel");
  addLookupRow("CANALS", chart_data::GeometryType::kLine, S52CompiledInstructionType::kLineStyle, "FAIRWY01", "line/channel");
  addLookupRow("RIVERS", chart_data::GeometryType::kLine, S52CompiledInstructionType::kLineStyle, "FAIRWY01", "line/channel");

  addLookupRow("DEPARE", chart_data::GeometryType::kArea, S52CompiledInstructionType::kAreaPattern, "DEPARE01", "area/depth");
  addLookupRow("DRGARE", chart_data::GeometryType::kArea, S52CompiledInstructionType::kAreaPattern, "DEPARE01", "area/depth");
  addLookupRow("LNDARE", chart_data::GeometryType::kArea, S52CompiledInstructionType::kAreaPattern, "LNDARE01", "area/land");
  addLookupRow("RESARE", chart_data::GeometryType::kArea, S52CompiledInstructionType::kAreaPattern, "RESARE01", "area/restricted");
  addLookupRow("UNSARE", chart_data::GeometryType::kArea, S52CompiledInstructionType::kAreaPattern, "RESARE01", "area/restricted");

  return catalog;
}

S52CompiledCatalog S52SourceCatalogCompiler::compile(const S52SourceCatalog &sourceCatalog)
{
  S52CompiledCatalog compiled;

  compiled.colors.reserve(sourceCatalog.colors.size());
  for(const auto &sourceColor : sourceCatalog.colors) {
    if(sourceColor.palette != S52PaletteId::kDay || sourceColor.token.empty()) {
      continue;
    }

    compiled.colors.push_back({normalizeToken(sourceColor.token), sourceColor.color});
  }

  compiled.pointSymbols.reserve(sourceCatalog.pointSymbols.size());
  for(const auto &sourceSymbol : sourceCatalog.pointSymbols) {
    if(sourceSymbol.assetId.empty() || sourceSymbol.colorToken.empty()) {
      continue;
    }

    compiled.pointSymbols.push_back({
      normalizeToken(sourceSymbol.assetId),
      normalizeToken(sourceSymbol.colorToken),
      sourceSymbol.radius});
  }

  compiled.lineStyles.reserve(sourceCatalog.lineStyles.size());
  for(const auto &sourceLine : sourceCatalog.lineStyles) {
    if(sourceLine.assetId.empty() || sourceLine.colorToken.empty()) {
      continue;
    }

    compiled.lineStyles.push_back({
      normalizeToken(sourceLine.assetId),
      normalizeToken(sourceLine.colorToken),
      sourceLine.thickness});
  }

  compiled.areaPatterns.reserve(sourceCatalog.areaPatterns.size());
  for(const auto &sourceArea : sourceCatalog.areaPatterns) {
    if(sourceArea.assetId.empty() || sourceArea.fillColorToken.empty()
       || sourceArea.outlineColorToken.empty() || sourceArea.holeFillColorToken.empty()) {
      continue;
    }

    compiled.areaPatterns.push_back({
      normalizeToken(sourceArea.assetId),
      normalizeToken(sourceArea.fillColorToken),
      normalizeToken(sourceArea.outlineColorToken),
      normalizeToken(sourceArea.holeFillColorToken),
      sourceArea.outlineThickness,
      sourceArea.fillAlpha});
  }

  compiled.lookupRows.reserve(sourceCatalog.lookupRows.size());
  for(const auto &sourceRow : sourceCatalog.lookupRows) {
    if(sourceRow.objectAcronym.empty() || sourceRow.instructions.empty()) {
      continue;
    }

    S52CompiledLookupRow compiledRow;
    compiledRow.ruleId = makeStableRuleId(sourceRow);
    compiledRow.objectAcronym = normalizeToken(sourceRow.objectAcronym);
    compiledRow.geometryType = sourceRow.geometryType;
    compiledRow.displayCategory = sourceRow.displayCategory.empty()
                                    ? "standard"
                                    : normalizeRuleId(sourceRow.displayCategory);
    compiledRow.instructions.reserve(sourceRow.instructions.size());
    for(const auto &instruction : sourceRow.instructions) {
      if(instruction.assetId.empty()) {
        continue;
      }

      compiledRow.instructions.push_back({
        instruction.type,
        normalizeToken(instruction.assetId),
        normalizeRuleId(instruction.styleKey)});
    }

    if(!compiledRow.instructions.empty()) {
      compiled.lookupRows.push_back(std::move(compiledRow));
    }
  }

  std::sort(
    compiled.colors.begin(),
    compiled.colors.end(),
    [](const auto &lhs, const auto &rhs) { return lhs.token < rhs.token; });
  std::sort(
    compiled.pointSymbols.begin(),
    compiled.pointSymbols.end(),
    [](const auto &lhs, const auto &rhs) { return lhs.assetId < rhs.assetId; });
  std::sort(
    compiled.lineStyles.begin(),
    compiled.lineStyles.end(),
    [](const auto &lhs, const auto &rhs) { return lhs.assetId < rhs.assetId; });
  std::sort(
    compiled.areaPatterns.begin(),
    compiled.areaPatterns.end(),
    [](const auto &lhs, const auto &rhs) { return lhs.assetId < rhs.assetId; });
  std::sort(
    compiled.lookupRows.begin(),
    compiled.lookupRows.end(),
    [](const auto &lhs, const auto &rhs) {
      return std::tie(lhs.objectAcronym, lhs.geometryType, lhs.ruleId)
           < std::tie(rhs.objectAcronym, rhs.geometryType, rhs.ruleId);
    });

  return compiled;
}

S52CompiledCatalog S52SourceCatalogCompiler::compileBuiltin()
{
  return compile(buildBuiltinS52SourceCatalog());
}

} // namespace chart_view::runtime::portrayal
