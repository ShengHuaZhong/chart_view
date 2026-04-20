#include "e400_dai_source_catalog_bridge.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <fstream>
#include <optional>
#include <sstream>
#include <string_view>
#include <vector>

namespace chart_view::runtime::portrayal {

namespace {

constexpr char kUnitSeparator = '\x1f';

struct DaiRecord
{
  std::string tag;
  std::string payload;
  std::size_t lineNumber{0};
};

std::string trimCopy(std::string_view value)
{
  while(!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
    value.remove_prefix(1);
  }

  while(!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
    value.remove_suffix(1);
  }

  return std::string(value);
}

std::string trimTrailingSeparators(std::string_view value)
{
  while(!value.empty() && (value.back() == '\r' || value.back() == '\n' || value.back() == kUnitSeparator)) {
    value.remove_suffix(1);
  }
  return std::string(value);
}

std::string upperAscii(std::string_view value)
{
  std::string normalized;
  normalized.reserve(value.size());
  for(const auto ch : value) {
    normalized.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
  }
  return normalized;
}

std::vector<std::string> splitUnitFields(std::string_view value)
{
  std::vector<std::string> fields;
  std::size_t start = 0;
  for(std::size_t index = 0; index < value.size(); ++index) {
    if(value[index] != kUnitSeparator) {
      continue;
    }

    fields.push_back(trimCopy(value.substr(start, index - start)));
    start = index + 1;
  }

  if(start <= value.size()) {
    fields.push_back(trimCopy(value.substr(start)));
  }

  std::erase_if(fields, [](const std::string &field) { return field.empty(); });
  return fields;
}

std::string firstUnitField(std::string_view value)
{
  const auto fields = splitUnitFields(value);
  return fields.empty() ? trimTrailingSeparators(value) : fields.front();
}

std::optional<DaiRecord> parseRecordLine(std::string line, std::size_t lineNumber)
{
  while(!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
    line.pop_back();
  }

  if(line.empty()) {
    return std::nullopt;
  }

  if(line.size() < 4) {
    return std::nullopt;
  }

  DaiRecord record;
  record.tag = upperAscii(line.substr(0, 4));
  record.lineNumber = lineNumber;
  record.payload = line.size() > 9 ? line.substr(9) : std::string();
  return record;
}

S52PaletteId paletteFromTableName(std::string_view tableName)
{
  const auto normalized = upperAscii(tableName);
  if(normalized == "DUSK") {
    return S52PaletteId::kDusk;
  }
  if(normalized == "NIGHT") {
    return S52PaletteId::kNight;
  }
  return S52PaletteId::kDay;
}

SurfaceColor cieToSrgb(double x, double y, double luminance)
{
  if(y <= 0.0) {
    return {0U, 0U, 0U, 255U};
  }

  const auto Y = std::clamp(luminance / 100.0, 0.0, 1.0);
  const auto X = x * (Y / y);
  const auto Z = (1.0 - x - y) * (Y / y);

  auto gammaEncode = [](double value) {
    value = std::max(value, 0.0);
    if(value <= 0.0031308) {
      return 12.92 * value;
    }
    return 1.055 * std::pow(value, 1.0 / 2.4) - 0.055;
  };

  const auto r = gammaEncode((3.2406 * X) + (-1.5372 * Y) + (-0.4986 * Z));
  const auto g = gammaEncode((-0.9689 * X) + (1.8758 * Y) + (0.0415 * Z));
  const auto b = gammaEncode((0.0557 * X) + (-0.2040 * Y) + (1.0570 * Z));

  auto toChannel = [](double value) {
    const auto scaled = std::lround(std::clamp(value, 0.0, 1.0) * 255.0);
    const auto bounded = std::max<long long>(0LL, std::min<long long>(255LL, scaled));
    return static_cast<std::uint8_t>(bounded);
  };

  return SurfaceColor{toChannel(r), toChannel(g), toChannel(b), 255U};
}

std::vector<int> parseFiveDigitGroups(std::string_view value)
{
  std::vector<int> groups;
  std::string digitsOnly;
  digitsOnly.reserve(value.size());
  for(const auto ch : value) {
    if(std::isdigit(static_cast<unsigned char>(ch)) != 0) {
      digitsOnly.push_back(ch);
    }
  }

  for(std::size_t index = 0; index + 4 < digitsOnly.size(); index += 5) {
    groups.push_back(std::stoi(digitsOnly.substr(index, 5)));
  }

  return groups;
}

void applySixGroupMetrics(const std::vector<int> &groups, S52SourceGraphicMetrics &metrics)
{
  if(groups.size() < 6) {
    return;
  }

  metrics.pivot = {groups[0], groups[1], true};
  metrics.width = groups[2];
  metrics.height = groups[3];
  metrics.origin = {groups[4], groups[5], true};
}

void applyPatternMetrics(const std::vector<int> &groups, S52SourceGraphicMetrics &metrics)
{
  if(groups.size() >= 6) {
    metrics.pivot = {groups[groups.size() - 6], groups[groups.size() - 5], true};
  }

  if(groups.size() >= 4) {
    metrics.width = groups[groups.size() - 4];
    metrics.height = groups[groups.size() - 3];
    metrics.origin = {groups[groups.size() - 2], groups[groups.size() - 1], true};
  }
}

std::string parseSourceRecordId(std::string_view payload)
{
  const auto trimmed = trimTrailingSeparators(payload);
  return trimmed.size() >= 7 ? trimmed.substr(0, 7) : trimmed;
}

std::string parseColsTableName(std::string_view payload)
{
  auto value = trimTrailingSeparators(payload);
  const auto marker = value.find("NIL");
  if(marker == std::string::npos) {
    return value;
  }

  value.erase(0, marker + 3);
  return trimTrailingSeparators(value);
}

void parseLbid(std::string_view payload, S52SourceCatalogProvenance &provenance)
{
  provenance.rawHeader = trimTrailingSeparators(payload);
  provenance.sourceFormat = "iho.preslib.dai";

  const auto fields = splitUnitFields(payload);
  if(fields.size() >= 2) {
    provenance.sourceRevision = fields[1];
  }
  if(fields.size() >= 3) {
    provenance.sourceEdition = fields[2];
  }
  if(fields.size() >= 4) {
    const auto &descriptor = fields[3];
    const auto officialPos = descriptor.find("Official");
    provenance.sourceDescription =
      officialPos == std::string::npos ? descriptor : descriptor.substr(officialPos);
    if(descriptor.size() >= 14) {
      provenance.sourceIssueDate = descriptor.substr(0, 14);
    }
  }

  const auto header = fields.empty() ? trimTrailingSeparators(payload) : fields.front();
  if(header.size() >= 7) {
    provenance.sourceIdentifier = header.substr(0, 7);
  }
  if(header.size() >= 10) {
    provenance.sourceAgency = header.substr(header.size() - 3);
  }
}

void parseCcie(std::string_view payload,
               S52PaletteId palette,
               const std::string &tableName,
               S52SourceCatalog &catalog)
{
  const auto value = trimTrailingSeparators(payload);
  if(value.size() < 11) {
    return;
  }

  const auto fields = splitUnitFields(value.substr(5));
  if(fields.size() < 3) {
    return;
  }

  const auto x = std::stod(fields[0]);
  const auto y = std::stod(fields[1]);
  const auto luminance = std::stod(fields[2]);

  S52SourceColor color;
  color.palette = palette;
  color.tableName = tableName;
  color.token = value.substr(0, 5);
  color.color = cieToSrgb(x, y, luminance);
  catalog.colors.push_back(std::move(color));
}

void parseSymd(std::string_view payload, S52SourcePointSymbol &symbol)
{
  const auto value = trimTrailingSeparators(payload);
  if(value.size() < 9) {
    return;
  }

  symbol.assetId = value.substr(0, 8);
  symbol.definition = value.substr(8, 1);
  applySixGroupMetrics(parseFiveDigitGroups(value.substr(9)), symbol.vectorMetrics);
}

void finalizePointSymbol(std::optional<S52SourcePointSymbol> &symbol, S52SourceCatalog &catalog)
{
  if(!symbol.has_value() || symbol->assetId.empty()) {
    symbol.reset();
    return;
  }

  if(symbol->radius <= 0) {
    const auto dominantSize = std::max(symbol->vectorMetrics.width, symbol->vectorMetrics.height);
    symbol->radius = dominantSize > 0 ? std::max(2, dominantSize / 8) : 4;
  }

  catalog.pointSymbols.push_back(std::move(*symbol));
  symbol.reset();
}

void parseLind(std::string_view payload, S52SourceLineStyle &lineStyle)
{
  const auto value = trimTrailingSeparators(payload);
  if(value.size() < 8) {
    return;
  }

  lineStyle.assetId = value.substr(0, 8);
  applySixGroupMetrics(parseFiveDigitGroups(value.substr(8)), lineStyle.vectorMetrics);
}

void finalizeLineStyle(std::optional<S52SourceLineStyle> &lineStyle, S52SourceCatalog &catalog)
{
  if(!lineStyle.has_value() || lineStyle->assetId.empty()) {
    lineStyle.reset();
    return;
  }

  if(lineStyle->thickness <= 0) {
    lineStyle->thickness =
      lineStyle->vectorMetrics.height > 0 ? std::max(1, lineStyle->vectorMetrics.height / 250) : 2;
  }

  catalog.lineStyles.push_back(std::move(*lineStyle));
  lineStyle.reset();
}

void parsePatd(std::string_view payload, S52SourceAreaPattern &pattern)
{
  const auto value = trimTrailingSeparators(payload);
  if(value.size() < 9) {
    return;
  }

  pattern.assetId = value.substr(0, 8);
  pattern.definition = value.substr(8, 1);

  const auto tail = value.substr(9);
  std::size_t alphaPrefixLength = 0;
  while(alphaPrefixLength < tail.size() && std::isalpha(static_cast<unsigned char>(tail[alphaPrefixLength])) != 0) {
    ++alphaPrefixLength;
  }

  const auto alphaPrefix = tail.substr(0, alphaPrefixLength);
  if(!alphaPrefix.empty()) {
    pattern.fillType = alphaPrefix.substr(0, 1);
    pattern.spacing = alphaPrefix.find('C') != std::string::npos ? "C" : std::string("C");
  } else {
    pattern.fillType = "S";
    pattern.spacing = "C";
  }

  applyPatternMetrics(parseFiveDigitGroups(tail), pattern.vectorMetrics);
}

void finalizePattern(std::optional<S52SourceAreaPattern> &pattern, S52SourceCatalog &catalog)
{
  if(!pattern.has_value() || pattern->assetId.empty()) {
    pattern.reset();
    return;
  }

  if(pattern->fillColorToken.empty()) {
    pattern->fillColorToken = pattern->primaryColorToken;
  }
  if(pattern->outlineColorToken.empty()) {
    pattern->outlineColorToken = pattern->primaryColorToken.empty() ? "NODTA" : pattern->primaryColorToken;
  }
  if(pattern->holeFillColorToken.empty()) {
    pattern->holeFillColorToken = "NODTA";
  }

  catalog.areaPatterns.push_back(std::move(*pattern));
  pattern.reset();
}

chart_data::GeometryType geometryTypeFromLuptCode(char code)
{
  switch(std::toupper(static_cast<unsigned char>(code))) {
  case 'L':
    return chart_data::GeometryType::kLine;
  case 'A':
    return chart_data::GeometryType::kArea;
  default:
    return chart_data::GeometryType::kPoint;
  }
}

std::string geometryTypeText(chart_data::GeometryType geometryType)
{
  switch(geometryType) {
  case chart_data::GeometryType::kLine:
    return "Line";
  case chart_data::GeometryType::kArea:
    return "Area";
  case chart_data::GeometryType::kPoint:
    return "Point";
  }

  return "Point";
}

std::string normalizeDisplayCategory(std::string_view value)
{
  auto normalized = trimTrailingSeparators(value);
  std::transform(
    normalized.begin(),
    normalized.end(),
    normalized.begin(),
    [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

  if(normalized.empty()) {
    return "Standard";
  }

  normalized.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(normalized.front())));
  return normalized;
}

std::uint32_t parseViewGroup(std::string_view value)
{
  std::string digits;
  for(const auto ch : value) {
    if(std::isdigit(static_cast<unsigned char>(ch)) != 0) {
      digits.push_back(ch);
    }
  }

  if(digits.empty()) {
    return 0U;
  }

  if(digits.size() > 5) {
    digits = digits.substr(digits.size() - 5);
  }

  return static_cast<std::uint32_t>(std::stoul(digits));
}

void parseLupt(std::string_view payload, S52SourceLookupRow &row)
{
  const auto value = trimTrailingSeparators(payload);
  if(value.size() < 23) {
    return;
  }

  row.sourceLookupId = value.substr(0, 7);
  row.sourceRcid = row.sourceLookupId;

  const auto core = value.substr(10);
  if(core.size() < 13) {
    return;
  }

  row.objectAcronym = trimCopy(core.substr(0, 6));
  row.geometryType = geometryTypeFromLuptCode(core[6]);
  row.geometryTypeText = geometryTypeText(row.geometryType);
  row.displayPriorityText = trimCopy(core.substr(7, 5));
  row.radarPriorityText = std::string(1, core[12]);
  row.tableName = trimTrailingSeparators(core.substr(13));
}

void finalizeLookupRow(std::optional<S52SourceLookupRow> &row, S52SourceCatalog &catalog)
{
  if(!row.has_value() || row->objectAcronym.empty()) {
    row.reset();
    return;
  }

  catalog.lookupRows.push_back(std::move(*row));
  row.reset();
}

} // namespace

E400DaiSourceCatalogParseResult E400DaiSourceCatalogBridge::parseFile(const std::filesystem::path &path)
{
  E400DaiSourceCatalogParseResult result;

  std::ifstream stream(path, std::ios::binary);
  if(!stream.is_open()) {
    result.error = "failed to open DAI file: " + path.string();
    return result;
  }

  result.catalog.provenance.sourcePath = path.string();

  std::optional<S52SourcePointSymbol> currentPointSymbol;
  std::optional<S52SourceLineStyle> currentLineStyle;
  std::optional<S52SourceAreaPattern> currentPattern;
  std::optional<S52SourceLookupRow> currentLookupRow;

  std::string currentColorTableName;
  S52PaletteId currentPalette{S52PaletteId::kDay};
  bool colorTableActive = false;

  std::string line;
  std::size_t lineNumber = 0;
  while(std::getline(stream, line)) {
    ++lineNumber;
    const auto parsedRecord = parseRecordLine(line, lineNumber);
    if(!parsedRecord.has_value()) {
      continue;
    }

    const auto &record = *parsedRecord;
    if(record.tag == "0001") {
      continue;
    }

    if(record.tag == "****") {
      finalizePointSymbol(currentPointSymbol, result.catalog);
      finalizeLineStyle(currentLineStyle, result.catalog);
      finalizePattern(currentPattern, result.catalog);
      finalizeLookupRow(currentLookupRow, result.catalog);
      colorTableActive = false;
      continue;
    }

    if(record.tag == "LBID") {
      parseLbid(record.payload, result.catalog.provenance);
      continue;
    }

    if(record.tag == "COLS") {
      colorTableActive = true;
      currentColorTableName = parseColsTableName(record.payload);
      currentPalette = paletteFromTableName(currentColorTableName);
      continue;
    }

    if(record.tag == "CCIE" && colorTableActive) {
      parseCcie(record.payload, currentPalette, currentColorTableName, result.catalog);
      continue;
    }

    if(record.tag == "SYMB") {
      finalizePointSymbol(currentPointSymbol, result.catalog);
      currentPointSymbol.emplace();
      currentPointSymbol->sourceRcid = parseSourceRecordId(record.payload);
      continue;
    }

    if(record.tag == "SYMD") {
      if(!currentPointSymbol.has_value()) {
        currentPointSymbol.emplace();
      }
      parseSymd(record.payload, *currentPointSymbol);
      continue;
    }

    if(record.tag == "SXPO" && currentPointSymbol.has_value()) {
      currentPointSymbol->description = firstUnitField(record.payload);
      continue;
    }

    if(record.tag == "SCRF" && currentPointSymbol.has_value()) {
      currentPointSymbol->colorToken = firstUnitField(record.payload);
      continue;
    }

    if(record.tag == "LNST") {
      finalizeLineStyle(currentLineStyle, result.catalog);
      currentLineStyle.emplace();
      currentLineStyle->sourceRcid = parseSourceRecordId(record.payload);
      continue;
    }

    if(record.tag == "LIND") {
      if(!currentLineStyle.has_value()) {
        currentLineStyle.emplace();
      }
      parseLind(record.payload, *currentLineStyle);
      continue;
    }

    if(record.tag == "LXPO" && currentLineStyle.has_value()) {
      currentLineStyle->description = firstUnitField(record.payload);
      continue;
    }

    if(record.tag == "LCRF" && currentLineStyle.has_value()) {
      currentLineStyle->colorToken = firstUnitField(record.payload);
      continue;
    }

    if(record.tag == "LVCT" && currentLineStyle.has_value()) {
      currentLineStyle->hpgl += firstUnitField(record.payload);
      continue;
    }

    if(record.tag == "PATT") {
      finalizePattern(currentPattern, result.catalog);
      currentPattern.emplace();
      currentPattern->sourceRcid = parseSourceRecordId(record.payload);
      continue;
    }

    if(record.tag == "PATD") {
      if(!currentPattern.has_value()) {
        currentPattern.emplace();
      }
      parsePatd(record.payload, *currentPattern);
      continue;
    }

    if(record.tag == "PXPO" && currentPattern.has_value()) {
      currentPattern->description = firstUnitField(record.payload);
      continue;
    }

    if(record.tag == "PCRF" && currentPattern.has_value()) {
      currentPattern->primaryColorToken = firstUnitField(record.payload);
      currentPattern->fillColorToken = currentPattern->primaryColorToken;
      currentPattern->outlineColorToken = currentPattern->primaryColorToken;
      continue;
    }

    if(record.tag == "PVCT" && currentPattern.has_value()) {
      currentPattern->hpgl += firstUnitField(record.payload);
      continue;
    }

    if(record.tag == "LUPT") {
      finalizeLookupRow(currentLookupRow, result.catalog);
      currentLookupRow.emplace();
      parseLupt(record.payload, *currentLookupRow);
      continue;
    }

    if(record.tag == "ATTC" && currentLookupRow.has_value()) {
      for(const auto &field : splitUnitFields(record.payload)) {
        if(!field.empty()) {
          currentLookupRow->attributeCodes.push_back(field);
        }
      }
      continue;
    }

    if(record.tag == "INST" && currentLookupRow.has_value()) {
      const auto instruction = firstUnitField(record.payload);
      if(!instruction.empty()) {
        if(!currentLookupRow->rawInstruction.empty()) {
          currentLookupRow->rawInstruction += ";";
        }
        currentLookupRow->rawInstruction += instruction;
      }
      continue;
    }

    if(record.tag == "DISC" && currentLookupRow.has_value()) {
      currentLookupRow->displayCategory = normalizeDisplayCategory(firstUnitField(record.payload));
      continue;
    }

    if(record.tag == "LUCM" && currentLookupRow.has_value()) {
      currentLookupRow->comment = firstUnitField(record.payload);
      currentLookupRow->viewGroup = parseViewGroup(currentLookupRow->comment);
      continue;
    }
  }

  finalizePointSymbol(currentPointSymbol, result.catalog);
  finalizeLineStyle(currentLineStyle, result.catalog);
  finalizePattern(currentPattern, result.catalog);
  finalizeLookupRow(currentLookupRow, result.catalog);

  if(result.catalog.colors.empty()) {
    result.error = "DAI parse produced no colors";
    return result;
  }
  if(result.catalog.pointSymbols.empty()) {
    result.error = "DAI parse produced no point symbols";
    return result;
  }
  if(result.catalog.lineStyles.empty()) {
    result.error = "DAI parse produced no line styles";
    return result;
  }
  if(result.catalog.areaPatterns.empty()) {
    result.error = "DAI parse produced no area patterns";
    return result;
  }
  if(result.catalog.lookupRows.empty()) {
    result.error = "DAI parse produced no lookup rows";
    return result;
  }

  result.ok = true;
  return result;
}

} // namespace chart_view::runtime::portrayal
