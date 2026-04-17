#include "s52_source_catalog_compiler.hpp"

#include "opencpn_chartsymbols_parser.hpp"
#include "s52_instruction_string_parser.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <optional>
#include <string_view>
#include <tuple>
#include <utility>

namespace chart_view::runtime::portrayal {

namespace {

std::filesystem::path defaultOpenCpnBundleRoot()
{
#ifdef CHART_VIEW_PHASE6A_OPENCPN_S57DATA_ROOT
  return std::filesystem::path(CHART_VIEW_PHASE6A_OPENCPN_S57DATA_ROOT);
#else
  const auto sourceFile = std::filesystem::path(__FILE__);
  return sourceFile.parent_path().parent_path().parent_path().parent_path() / "vendor" / "opencpn_s57data"
       / "Release_5.14.0" / "s57data";
#endif
}

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

std::string instructionTypeToken(S52InstructionType type)
{
  switch(type) {
  case S52InstructionType::kPointSymbol:
    return "point";
  case S52InstructionType::kLineStyle:
    return "line";
  case S52InstructionType::kAreaPattern:
    return "area";
  case S52InstructionType::kTextLabel:
    return "text";
  case S52InstructionType::kConditional:
    return "conditional";
  }

  return "unknown";
}

int defaultDisplayPriority(chart_data::GeometryType geometryType) noexcept
{
  switch(geometryType) {
  case chart_data::GeometryType::kPoint:
    return 300;
  case chart_data::GeometryType::kLine:
    return 200;
  case chart_data::GeometryType::kArea:
    return 100;
  }

  return 0;
}

std::uint32_t defaultViewGroup(chart_data::GeometryType geometryType) noexcept
{
  switch(geometryType) {
  case chart_data::GeometryType::kPoint:
    return 33010U;
  case chart_data::GeometryType::kLine:
    return 23010U;
  case chart_data::GeometryType::kArea:
    return 13010U;
  }

  return 0U;
}

std::string makeStableRuleId(const S52SourceLookupRow &row)
{
  if(!row.ruleId.empty()) {
    return normalizeRuleId(row.ruleId);
  }

  if(!row.sourceRcid.empty() || !row.sourceLookupId.empty()) {
    std::string stableId = "s52_";
    stableId += geometryTypeToken(row.geometryType);
    stableId += "_";
    stableId += normalizeRuleId(row.objectAcronym);
    if(!row.tableName.empty()) {
      stableId += "_";
      stableId += normalizeRuleId(row.tableName);
    }
    if(!row.sourceRcid.empty()) {
      stableId += "_rcid_";
      stableId += normalizeRuleId(row.sourceRcid);
    } else {
      stableId += "_id_";
      stableId += normalizeRuleId(row.sourceLookupId);
    }
    return stableId;
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

std::optional<S52Instruction> compileInstruction(const S52SourceLookupInstruction &instruction)
{
  if(instruction.assetId.empty() && instruction.type != S52InstructionType::kConditional) {
    return std::nullopt;
  }

  const auto assetId = normalizeToken(instruction.assetId);
  const auto styleKey = std::string(instruction.styleKey);
  switch(instruction.type) {
  case S52InstructionType::kPointSymbol:
    return S52PointSymbolInstruction{assetId, styleKey};
  case S52InstructionType::kLineStyle:
    return S52LineStyleInstruction{assetId, styleKey};
  case S52InstructionType::kAreaPattern:
    return S52AreaPatternInstruction{assetId, styleKey};
  case S52InstructionType::kTextLabel:
    return S52TextInstruction{
      assetId,
      styleKey,
      instruction.attributeKey.empty() ? std::string("OBJNAM") : instruction.attributeKey};
  case S52InstructionType::kConditional:
    return makeConditionalInstruction(instruction.styleKey.empty() ? instruction.assetId : instruction.styleKey);
  }

  return std::nullopt;
}

SurfaceColor makeColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255U)
{
  return {r, g, b, a};
}

std::string normalizeOptionalToken(std::string_view value)
{
  const auto normalized = normalizeToken(value);
  return normalized.empty() ? std::string{} : normalized;
}

template <typename T>
bool hasAssetWithId(const std::vector<T> &assets, std::string_view assetId)
{
  return std::any_of(assets.begin(), assets.end(), [&](const auto &asset) { return asset.assetId == assetId; });
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
                                S52InstructionType instructionType,
                                std::string_view assetId,
                                std::string_view styleKey) {
    S52SourceLookupRow row;
    row.objectAcronym = std::string(objectAcronym);
    row.geometryType = geometryType;
    row.displayPriority = defaultDisplayPriority(geometryType);
    row.viewGroup = defaultViewGroup(geometryType);
    row.instructions.push_back(
      {instructionType, std::string(assetId), std::string(styleKey)});
    catalog.lookupRows.push_back(std::move(row));
  };

  addLookupRow("SOUNDG", chart_data::GeometryType::kPoint, S52InstructionType::kPointSymbol, "SOUNDG01", "point/sounding");
  addLookupRow("BOYSPP", chart_data::GeometryType::kPoint, S52InstructionType::kPointSymbol, "BOYSPP01", "point/buoy");
  addLookupRow("BOYLAT", chart_data::GeometryType::kPoint, S52InstructionType::kPointSymbol, "BOYSPP01", "point/buoy");
  addLookupRow("BOYSAW", chart_data::GeometryType::kPoint, S52InstructionType::kPointSymbol, "BOYSPP01", "point/buoy");
  addLookupRow("BCNSPP", chart_data::GeometryType::kPoint, S52InstructionType::kPointSymbol, "BCNSPP01", "point/beacon");
  addLookupRow("BCNLAT", chart_data::GeometryType::kPoint, S52InstructionType::kPointSymbol, "BCNSPP01", "point/beacon");
  addLookupRow("BCNSAW", chart_data::GeometryType::kPoint, S52InstructionType::kPointSymbol, "BCNSPP01", "point/beacon");
  addLookupRow("WRECKS", chart_data::GeometryType::kPoint, S52InstructionType::kPointSymbol, "DANGER01", "point/danger");
  addLookupRow("UWTROC", chart_data::GeometryType::kPoint, S52InstructionType::kPointSymbol, "DANGER01", "point/danger");
  addLookupRow("LIGHTS", chart_data::GeometryType::kPoint, S52InstructionType::kPointSymbol, "LNDMRK01", "point/landmark");
  addLookupRow("LNDMRK", chart_data::GeometryType::kPoint, S52InstructionType::kPointSymbol, "LNDMRK01", "point/landmark");
  addLookupRow("PILPNT", chart_data::GeometryType::kPoint, S52InstructionType::kPointSymbol, "LNDMRK01", "point/landmark");

  addLookupRow("DEPCNT", chart_data::GeometryType::kLine, S52InstructionType::kLineStyle, "DEPCN01", "line/depth_contour");
  addLookupRow("COALNE", chart_data::GeometryType::kLine, S52InstructionType::kLineStyle, "COALNE01", "line/coastline");
  addLookupRow("FAIRWY", chart_data::GeometryType::kLine, S52InstructionType::kLineStyle, "FAIRWY01", "line/channel");
  addLookupRow("CANALS", chart_data::GeometryType::kLine, S52InstructionType::kLineStyle, "FAIRWY01", "line/channel");
  addLookupRow("RIVERS", chart_data::GeometryType::kLine, S52InstructionType::kLineStyle, "FAIRWY01", "line/channel");

  addLookupRow("DEPARE", chart_data::GeometryType::kArea, S52InstructionType::kAreaPattern, "DEPARE01", "area/depth");
  addLookupRow("DRGARE", chart_data::GeometryType::kArea, S52InstructionType::kAreaPattern, "DEPARE01", "area/depth");
  addLookupRow("LNDARE", chart_data::GeometryType::kArea, S52InstructionType::kAreaPattern, "LNDARE01", "area/land");
  addLookupRow("RESARE", chart_data::GeometryType::kArea, S52InstructionType::kAreaPattern, "RESARE01", "area/restricted");
  addLookupRow("UNSARE", chart_data::GeometryType::kArea, S52InstructionType::kAreaPattern, "RESARE01", "area/restricted");

  return catalog;
}

S52CompiledCatalog S52SourceCatalogCompiler::compile(const S52SourceCatalog &sourceCatalog,
                                                     std::string_view catalogIdHint)
{
  S52CompiledCatalog compiled;
  compiled.catalogId = std::string(catalogIdHint);

  compiled.colors.reserve(sourceCatalog.colors.size());
  for(const auto &sourceColor : sourceCatalog.colors) {
    if(sourceColor.token.empty()) {
      continue;
    }

    compiled.colors.push_back(
      {normalizeToken(sourceColor.token), sourceColor.color, sourceColor.palette, sourceColor.tableName});
  }

  compiled.pointSymbols.reserve(sourceCatalog.pointSymbols.size());
  for(const auto &sourceSymbol : sourceCatalog.pointSymbols) {
    if(sourceSymbol.assetId.empty() || sourceSymbol.colorToken.empty()) {
      continue;
    }

    compiled.pointSymbols.push_back({
      normalizeToken(sourceSymbol.assetId),
      normalizeToken(sourceSymbol.colorToken),
      sourceSymbol.radius,
      sourceSymbol.sourceRcid,
      sourceSymbol.description,
      sourceSymbol.bitmapMetrics,
      sourceSymbol.vectorMetrics,
      sourceSymbol.preferBitmap});
  }

  compiled.lineStyles.reserve(sourceCatalog.lineStyles.size());
  for(const auto &sourceLine : sourceCatalog.lineStyles) {
    if(sourceLine.assetId.empty() || sourceLine.colorToken.empty()) {
      continue;
    }

    compiled.lineStyles.push_back({
      normalizeToken(sourceLine.assetId),
      normalizeToken(sourceLine.colorToken),
      sourceLine.thickness,
      sourceLine.sourceRcid,
      sourceLine.description,
      sourceLine.hpgl,
      sourceLine.vectorMetrics});
  }

  compiled.areaPatterns.reserve(sourceCatalog.areaPatterns.size());
  for(const auto &sourceArea : sourceCatalog.areaPatterns) {
    const auto fillColorToken =
      !sourceArea.fillColorToken.empty() ? sourceArea.fillColorToken : sourceArea.primaryColorToken;
    const auto outlineColorToken =
      !sourceArea.outlineColorToken.empty()
        ? sourceArea.outlineColorToken
        : (!sourceArea.primaryColorToken.empty() ? sourceArea.primaryColorToken : fillColorToken);
    const auto holeFillColorToken =
      !sourceArea.holeFillColorToken.empty() ? sourceArea.holeFillColorToken : std::string("NODTA");
    if(sourceArea.assetId.empty() || fillColorToken.empty() || outlineColorToken.empty()
       || holeFillColorToken.empty()) {
      continue;
    }

    compiled.areaPatterns.push_back({
      normalizeToken(sourceArea.assetId),
      normalizeToken(fillColorToken),
      normalizeToken(outlineColorToken),
      normalizeToken(holeFillColorToken),
      sourceArea.outlineThickness,
      sourceArea.fillAlpha,
      sourceArea.sourceRcid,
      sourceArea.description,
      normalizeOptionalToken(sourceArea.fillType),
      normalizeOptionalToken(sourceArea.spacing),
      sourceArea.hpgl,
      normalizeOptionalToken(sourceArea.primaryColorToken),
      sourceArea.bitmapMetrics,
      sourceArea.vectorMetrics});
  }

  compiled.lookupRows.reserve(sourceCatalog.lookupRows.size());
  for(const auto &sourceRow : sourceCatalog.lookupRows) {
    if(sourceRow.objectAcronym.empty()) {
      continue;
    }

    S52CompiledLookupRow compiledRow;
    compiledRow.ruleId = makeStableRuleId(sourceRow);
    compiledRow.objectAcronym = normalizeToken(sourceRow.objectAcronym);
    compiledRow.geometryType = sourceRow.geometryType;
    compiledRow.displayCategory = sourceRow.displayCategory.empty()
                                    ? "standard"
                                    : normalizeRuleId(sourceRow.displayCategory);
    compiledRow.displayPriority = sourceRow.displayPriority;
    compiledRow.viewGroup = sourceRow.viewGroup;
    compiledRow.sourceLookupId = sourceRow.sourceLookupId;
    compiledRow.sourceRcid = sourceRow.sourceRcid;
    compiledRow.tableName = sourceRow.tableName;
    compiledRow.radarPriorityText = sourceRow.radarPriorityText;
    compiledRow.attributeCodes = sourceRow.attributeCodes;
    compiledRow.rawInstruction = sourceRow.rawInstruction;
    const auto parsedInstructionResult =
      sourceRow.instructions.empty() && !sourceRow.rawInstruction.empty()
        ? S52InstructionStringParser::parse(sourceRow.rawInstruction)
        : S52InstructionStringParseResult{};
    const auto &sourceInstructions =
      sourceRow.instructions.empty() ? parsedInstructionResult.instructions : sourceRow.instructions;

    compiledRow.instructions.reserve(sourceInstructions.size());
    for(const auto &instruction : sourceInstructions) {
      if(const auto compiledInstruction = compileInstruction(instruction); compiledInstruction.has_value()) {
        compiledRow.instructions.push_back(*compiledInstruction);
      }
    }

    if(!compiledRow.instructions.empty() || !compiledRow.rawInstruction.empty() || !compiledRow.sourceRcid.empty()) {
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
  return compile(buildBuiltinS52SourceCatalog(), "builtin.private");
}

S52CompiledCatalog S52SourceCatalogCompiler::compileOpenCpnBundle(const OpenCpnS52ResourceBundle &bundle,
                                                                  std::string *error)
{
  const auto parseResult = OpenCpnChartsymbolsParser::parseBundle(bundle);
  if(!parseResult.ok) {
    if(error != nullptr) {
      *error = parseResult.error;
    }
    return {};
  }

  if(error != nullptr) {
    error->clear();
  }

  return compile(parseResult.catalog, "opencpn.release_5_14_0");
}

S52CompiledCatalog S52SourceCatalogCompiler::compilePreferred()
{
  static const auto preferredCatalog = [] {
    const auto root = defaultOpenCpnBundleRoot();
    if(std::filesystem::exists(root / "chartsymbols.xml")) {
      std::string error;
      if(auto compiled = compileOpenCpnBundle({root}, &error);
         !compiled.lookupRows.empty() || !compiled.colors.empty()) {
        return compiled;
      }
    }

    return compileBuiltin();
  }();

  return preferredCatalog;
}

} // namespace chart_view::runtime::portrayal
