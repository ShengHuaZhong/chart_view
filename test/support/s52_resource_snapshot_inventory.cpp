#include "s52_resource_snapshot_inventory.hpp"

#include "portrayal/opencpn_chartsymbols_parser.hpp"
#include "portrayal/opencpn_s52_resource_bundle.hpp"
#include "portrayal/s52_compiled_catalog.hpp"
#include "portrayal/s52_conditional_opcode.hpp"
#include "portrayal/s52_instruction_ir.hpp"
#include "portrayal/s52_instruction_string_parser.hpp"
#include "portrayal/s52_source_catalog.hpp"
#include "portrayal/s52_source_catalog_compiler.hpp"

#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace chart_view::test_support {

namespace {

using chart_view::runtime::chart_data::GeometryType;
using chart_view::runtime::portrayal::OpenCpnChartsymbolsParser;
using chart_view::runtime::portrayal::OpenCpnS52ResourceBundle;
using chart_view::runtime::portrayal::S52CompiledCatalog;
using chart_view::runtime::portrayal::S52CompiledLookupRow;
using chart_view::runtime::portrayal::S52ConditionalOpcode;
using chart_view::runtime::portrayal::S52InstructionStringParseResult;
using chart_view::runtime::portrayal::S52InstructionStringParser;
using chart_view::runtime::portrayal::S52InstructionType;
using chart_view::runtime::portrayal::S52SourceCatalogCompiler;
using chart_view::runtime::portrayal::S52SourceLookupInstruction;
using chart_view::runtime::portrayal::conditionalOpcodeToken;
using chart_view::runtime::portrayal::parseConditionalOpcode;

enum class CoverageStatus
{
  kSupported,
  kPartial,
  kUnsupported,
};

struct StatementTokenStats
{
  int parsedCount{0};
  int unsupportedCount{0};
};

struct ConditionalTokenStats
{
  int supportedCount{0};
  int unsupportedCount{0};
};

struct RowAnalysis
{
  CoverageStatus status{CoverageStatus::kSupported};
  std::string objectAcronym;
  std::string geometryType;
  std::string tableName;
  std::string sourceLookupId;
  std::string sourceRcid;
  std::string ruleId;
  std::vector<std::string> reasons;
  std::vector<std::string> instructionTokens;
  std::vector<std::string> unsupportedInstructionTokens;
  std::vector<std::string> conditionalTokens;
  std::vector<std::string> unresolvedConditionalTokens;
  std::vector<std::string> pointAssets;
  std::vector<std::string> lineAssets;
  std::vector<std::string> areaAssets;
  std::vector<std::string> areaColorTokens;
  std::vector<std::string> textAssets;
  std::vector<std::string> textAttributeKeys;
  bool harnessCovered{false};
};

struct RowKey
{
  std::string objectAcronym;
  GeometryType geometryType{GeometryType::kPoint};
  std::string tableName;
  std::string sourceLookupId;
  std::string sourceRcid;
};

[[nodiscard]] bool operator<(const RowKey &lhs, const RowKey &rhs) noexcept
{
  return std::tie(
           lhs.objectAcronym,
           lhs.geometryType,
           lhs.tableName,
           lhs.sourceLookupId,
           lhs.sourceRcid)
       < std::tie(
           rhs.objectAcronym,
           rhs.geometryType,
           rhs.tableName,
           rhs.sourceLookupId,
           rhs.sourceRcid);
}

[[nodiscard]] std::filesystem::path bundleRoot(const std::filesystem::path &projectSourceDir)
{
  return projectSourceDir / "vendor" / "opencpn_s57data" / "Release_5.14.0" / "s57data";
}

[[nodiscard]] std::vector<std::filesystem::path> referenceScenePaths(
  const std::filesystem::path &projectSourceDir)
{
  std::vector<std::filesystem::path> paths;
  const auto referenceRoot = projectSourceDir / "tests" / "data" / "reference";
  if(!std::filesystem::exists(referenceRoot)) {
    return paths;
  }

  for(const auto &entry : std::filesystem::directory_iterator(referenceRoot)) {
    if(!entry.is_regular_file()) {
      continue;
    }

    const auto fileName = entry.path().filename().string();
    if(entry.path().extension() != ".json" || !fileName.ends_with(".reference.json")) {
      continue;
    }

    if(fileName == "phase6b_s52_resource_snapshot_inventory.reference.json") {
      continue;
    }

    paths.push_back(entry.path());
  }

  std::sort(paths.begin(), paths.end());
  return paths;
}

[[nodiscard]] std::string toStdString(const QString &value)
{
  return value.toStdString();
}

[[nodiscard]] std::string trimCopy(std::string_view value)
{
  while(!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
    value.remove_prefix(1);
  }

  while(!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
    value.remove_suffix(1);
  }

  return std::string(value);
}

[[nodiscard]] std::string upperAscii(std::string_view value)
{
  std::string normalized;
  normalized.reserve(value.size());
  for(const auto ch : value) {
    normalized.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
  }
  return normalized;
}

[[nodiscard]] std::string normalizeRowKeyField(std::string_view value)
{
  return trimCopy(value);
}

[[nodiscard]] std::string normalizeAssetReference(std::string_view value)
{
  std::string normalized;
  normalized.reserve(value.size());
  for(const auto ch : trimCopy(value)) {
    if(std::isalnum(static_cast<unsigned char>(ch)) != 0) {
      normalized.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    } else if(ch == '_' || ch == '-' || ch == '/' || ch == '.') {
      normalized.push_back('_');
    }
  }
  return normalized;
}

[[nodiscard]] RowKey makeNormalizedRowKey(std::string_view objectAcronym,
                                          GeometryType geometryType,
                                          std::string_view tableName,
                                          std::string_view sourceLookupId,
                                          std::string_view sourceRcid)
{
  return RowKey{
    upperAscii(trimCopy(objectAcronym)),
    geometryType,
    normalizeRowKeyField(tableName),
    normalizeRowKeyField(sourceLookupId),
    normalizeRowKeyField(sourceRcid)};
}

[[nodiscard]] std::vector<std::string> splitTopLevel(std::string_view value, char delimiter)
{
  std::vector<std::string> parts;
  std::size_t start = 0;
  int parenDepth = 0;
  bool inQuote = false;

  for(std::size_t index = 0; index < value.size(); ++index) {
    const auto ch = value[index];
    if(ch == '\'') {
      inQuote = !inQuote;
      continue;
    }

    if(inQuote) {
      continue;
    }

    if(ch == '(') {
      ++parenDepth;
      continue;
    }

    if(ch == ')') {
      parenDepth = std::max(parenDepth - 1, 0);
      continue;
    }

    if(ch == delimiter && parenDepth == 0) {
      parts.push_back(trimCopy(value.substr(start, index - start)));
      start = index + 1;
    }
  }

  if(start <= value.size()) {
    parts.push_back(trimCopy(value.substr(start)));
  }

  std::erase_if(parts, [](const std::string &part) { return part.empty(); });
  return parts;
}

[[nodiscard]] std::string extractOpcode(std::string_view statement)
{
  const auto open = statement.find('(');
  if(open == std::string_view::npos) {
    return upperAscii(trimCopy(statement));
  }
  return upperAscii(trimCopy(statement.substr(0, open)));
}

[[nodiscard]] std::string geometryTypeText(GeometryType geometryType)
{
  switch(geometryType) {
  case GeometryType::kPoint:
    return "Point";
  case GeometryType::kLine:
    return "Line";
  case GeometryType::kArea:
    return "Area";
  }

  return "Unknown";
}

[[nodiscard]] std::string statusText(CoverageStatus status)
{
  switch(status) {
  case CoverageStatus::kSupported:
    return "supported";
  case CoverageStatus::kPartial:
    return "partial";
  case CoverageStatus::kUnsupported:
    return "unsupported";
  }

  return "unsupported";
}

[[nodiscard]] std::string sha256OfFile(const std::filesystem::path &path)
{
  QFile file(QString::fromStdWString(path.wstring()));
  if(!file.open(QIODevice::ReadOnly)) {
    return {};
  }

  QCryptographicHash hash(QCryptographicHash::Sha256);
  while(!file.atEnd()) {
    hash.addData(file.read(64 * 1024));
  }

  return toStdString(hash.result().toHex());
}

[[nodiscard]] QJsonObject stringCountObject(const std::map<std::string, int> &counts, std::string_view keyName)
{
  QJsonArray entries;
  for(const auto &[key, count] : counts) {
    QJsonObject object;
    object.insert(QString::fromStdString(std::string(keyName)), QString::fromStdString(key));
    object.insert("count", count);
    entries.push_back(object);
  }

  QJsonObject root;
  root.insert("entries", entries);
  root.insert("totalDistinct", static_cast<int>(counts.size()));
  return root;
}

[[nodiscard]] QJsonObject tokenSupportObject(const std::map<std::string, StatementTokenStats> &stats)
{
  QJsonArray supported;
  QJsonArray partial;
  QJsonArray unsupported;

  for(const auto &[token, tokenStats] : stats) {
    QJsonObject entry;
    entry.insert("token", QString::fromStdString(token));
    entry.insert("parsedCount", tokenStats.parsedCount);
    entry.insert("unsupportedCount", tokenStats.unsupportedCount);

    if(tokenStats.parsedCount > 0 && tokenStats.unsupportedCount == 0) {
      supported.push_back(entry);
    } else if(tokenStats.parsedCount == 0 && tokenStats.unsupportedCount > 0) {
      unsupported.push_back(entry);
    } else {
      partial.push_back(entry);
    }
  }

  QJsonObject root;
  root.insert("supported", supported);
  root.insert("partial", partial);
  root.insert("unsupported", unsupported);
  return root;
}

[[nodiscard]] QJsonObject conditionalSupportObject(const std::map<std::string, ConditionalTokenStats> &stats)
{
  QJsonArray supported;
  QJsonArray unsupported;

  for(const auto &[token, tokenStats] : stats) {
    QJsonObject entry;
    entry.insert("token", QString::fromStdString(token));
    entry.insert("supportedCount", tokenStats.supportedCount);
    entry.insert("unsupportedCount", tokenStats.unsupportedCount);
    if(tokenStats.unsupportedCount == 0) {
      supported.push_back(entry);
    } else {
      unsupported.push_back(entry);
    }
  }

  QJsonObject root;
  root.insert("supported", supported);
  root.insert("unsupported", unsupported);
  return root;
}

[[nodiscard]] QJsonArray stringArray(const std::vector<std::string> &values)
{
  QJsonArray array;
  for(const auto &value : values) {
    array.push_back(QString::fromStdString(value));
  }
  return array;
}

[[nodiscard]] std::vector<std::string> sortedUnique(std::vector<std::string> values)
{
  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end()), values.end());
  return values;
}

void appendInstructionAssets(const S52SourceLookupInstruction &instruction,
                             std::vector<std::string> &pointAssets,
                             std::vector<std::string> &lineAssets,
                             std::vector<std::string> &areaAssets,
                             std::vector<std::string> &areaColorTokens,
                             std::vector<std::string> &textAssets,
                             std::vector<std::string> &textAttributeKeys,
                             std::vector<std::string> &conditionalTokens,
                             std::map<std::string, int> &pointAssetCounts,
                             std::map<std::string, int> &lineAssetCounts,
                             std::map<std::string, int> &areaAssetCounts,
                             std::map<std::string, int> &areaColorTokenCounts,
                             std::map<std::string, int> &textAssetCounts,
                             std::map<std::string, int> &textAttributeCounts,
                             std::map<std::string, ConditionalTokenStats> &conditionalTokenStats)
{
  switch(instruction.type) {
  case S52InstructionType::kPointSymbol:
    if(const auto assetId = normalizeAssetReference(instruction.assetId); !assetId.empty()) {
      pointAssets.push_back(assetId);
      ++pointAssetCounts[assetId];
    }
    break;
  case S52InstructionType::kLineStyle:
    if(const auto assetId = normalizeAssetReference(instruction.assetId); !assetId.empty()) {
      lineAssets.push_back(assetId);
      ++lineAssetCounts[assetId];
    }
    break;
  case S52InstructionType::kAreaPattern:
    if(const auto assetId = normalizeAssetReference(instruction.assetId); !assetId.empty()) {
      areaAssets.push_back(assetId);
      ++areaAssetCounts[assetId];
    }
    break;
  case S52InstructionType::kAreaColor:
    if(const auto assetId = normalizeAssetReference(instruction.assetId); !assetId.empty()) {
      areaColorTokens.push_back(assetId);
      ++areaColorTokenCounts[assetId];
    }
    break;
  case S52InstructionType::kTextLabel:
    if(const auto assetId = normalizeAssetReference(instruction.assetId); !assetId.empty()) {
      textAssets.push_back(assetId);
      ++textAssetCounts[assetId];
    }
    if(!instruction.attributeKey.empty()) {
      textAttributeKeys.push_back(instruction.attributeKey);
      ++textAttributeCounts[instruction.attributeKey];
    }
    break;
  case S52InstructionType::kConditional: {
    conditionalTokens.push_back(instruction.assetId);
    auto &stats = conditionalTokenStats[instruction.assetId];
    if(parseConditionalOpcode(instruction.assetId) == S52ConditionalOpcode::kUnknown) {
      ++stats.unsupportedCount;
    } else {
      ++stats.supportedCount;
    }
    break;
  }
  }
}

template <typename T>
[[nodiscard]] bool containsAssetId(const std::vector<T> &assets, std::string_view assetId)
{
  const auto normalizedAssetId = normalizeAssetReference(assetId);
  return std::any_of(
    assets.begin(),
    assets.end(),
    [&](const auto &asset) { return normalizeAssetReference(asset.assetId) == normalizedAssetId; });
}

[[nodiscard]] std::map<RowKey, const S52CompiledLookupRow *> compiledRowMap(const S52CompiledCatalog &compiledCatalog)
{
  std::map<RowKey, const S52CompiledLookupRow *> rows;
  for(const auto &row : compiledCatalog.lookupRows) {
    rows.emplace(
      makeNormalizedRowKey(
        row.objectAcronym,
        row.geometryType,
        row.tableName,
        row.sourceLookupId,
        row.sourceRcid),
      &row);
  }
  return rows;
}

void collectHarnessCoverage(const std::filesystem::path &projectSourceDir,
                            std::set<std::string> &coveredRuleIds,
                            std::set<std::string> &coveredSourceRcids,
                            std::vector<std::string> &sceneIds)
{
  for(const auto &path : referenceScenePaths(projectSourceDir)) {
    QFile file(QString::fromStdWString(path.wstring()));
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      continue;
    }

    const auto document = QJsonDocument::fromJson(file.readAll());
    const auto root = document.object();
    sceneIds.push_back(root.value("sceneId").toString().toStdString());
    const auto features = root.value("features").toArray();
    for(const auto &featureValue : features) {
      const auto feature = featureValue.toObject();
      const auto ruleId = feature.value("ruleId").toString().toStdString();
      if(!ruleId.empty()) {
        coveredRuleIds.insert(ruleId);
      }

      const auto sourceRcid = feature.value("sourceRcid").toString().toStdString();
      if(!sourceRcid.empty()) {
        coveredSourceRcids.insert(sourceRcid);
      }
    }
  }

  std::sort(sceneIds.begin(), sceneIds.end());
}

} // namespace

S52ResourceSnapshotInventoryResult buildS52ResourceSnapshotInventory(
  const std::filesystem::path &projectSourceDir)
{
  const auto sourceBundleRoot = bundleRoot(projectSourceDir);
  const OpenCpnS52ResourceBundle bundle{sourceBundleRoot};
  const auto parseResult = OpenCpnChartsymbolsParser::parseBundle(bundle);

  S52ResourceSnapshotInventoryResult result;
  if(!parseResult.ok) {
    QJsonObject error;
    error.insert("schema", "s52_resource_snapshot_inventory_v1");
    error.insert("ok", false);
    error.insert("error", QString::fromStdString(parseResult.error));
    result.document = QJsonDocument(error);
    return result;
  }

  const auto compiledCatalog = S52SourceCatalogCompiler::compile(parseResult.catalog, "opencpn.release_5_14_0");
  const auto compiledRows = compiledRowMap(compiledCatalog);

  std::set<std::string> coveredRuleIds;
  std::set<std::string> coveredSourceRcids;
  std::vector<std::string> sceneIds;
  collectHarnessCoverage(projectSourceDir, coveredRuleIds, coveredSourceRcids, sceneIds);

  std::map<std::string, int> objectCounts;
  std::map<std::string, int> geometryCounts;
  std::map<std::string, int> tableCounts;
  std::map<std::string, int> pointAssetCounts;
  std::map<std::string, int> lineAssetCounts;
  std::map<std::string, int> areaAssetCounts;
  std::map<std::string, int> areaColorTokenCounts;
  std::map<std::string, int> textAssetCounts;
  std::map<std::string, int> textAttributeCounts;
  std::map<std::string, StatementTokenStats> statementTokenStats;
  std::map<std::string, int> statementTokenCounts;
  std::map<std::string, ConditionalTokenStats> conditionalTokenStats;
  std::map<std::string, int> conditionalTokenCounts;
  std::vector<RowAnalysis> degradedRows;

  for(const auto &sourceRow : parseResult.catalog.lookupRows) {
    ++result.lookupRowsTotal;
    ++objectCounts[sourceRow.objectAcronym];
    ++geometryCounts[geometryTypeText(sourceRow.geometryType)];
    ++tableCounts[sourceRow.tableName.empty() ? std::string("(none)") : sourceRow.tableName];

    const auto parsedInstructions =
      sourceRow.instructions.empty() && !sourceRow.rawInstruction.empty()
        ? S52InstructionStringParser::parse(sourceRow.rawInstruction)
        : S52InstructionStringParseResult{};
    const auto &effectiveInstructions =
      sourceRow.instructions.empty() ? parsedInstructions.instructions : sourceRow.instructions;

    std::vector<std::string> statementTokens;
    if(!sourceRow.rawInstruction.empty()) {
      for(const auto &statement : splitTopLevel(sourceRow.rawInstruction, ';')) {
        const auto opcode = extractOpcode(statement);
        if(opcode.empty()) {
          continue;
        }

        statementTokens.push_back(opcode);
        ++statementTokenCounts[opcode];
      }
    }

    for(const auto &statement : parsedInstructions.unsupportedStatements) {
      const auto opcode = extractOpcode(statement);
      if(!opcode.empty()) {
        ++statementTokenStats[opcode].unsupportedCount;
      }
    }

    for(const auto &statementToken : statementTokens) {
      if(std::none_of(
           parsedInstructions.unsupportedStatements.begin(),
           parsedInstructions.unsupportedStatements.end(),
           [&](const std::string &statement) { return extractOpcode(statement) == statementToken; })) {
        ++statementTokenStats[statementToken].parsedCount;
      }
    }

    std::vector<std::string> pointAssets;
    std::vector<std::string> lineAssets;
    std::vector<std::string> areaAssets;
    std::vector<std::string> areaColorTokens;
    std::vector<std::string> textAssets;
    std::vector<std::string> textAttributeKeys;
    std::vector<std::string> conditionalTokens;
    for(const auto &instruction : effectiveInstructions) {
      appendInstructionAssets(
        instruction,
        pointAssets,
        lineAssets,
        areaAssets,
        areaColorTokens,
        textAssets,
        textAttributeKeys,
        conditionalTokens,
        pointAssetCounts,
        lineAssetCounts,
        areaAssetCounts,
        areaColorTokenCounts,
        textAssetCounts,
        textAttributeCounts,
        conditionalTokenStats);
    }

    for(const auto &token : conditionalTokens) {
      ++conditionalTokenCounts[token];
    }

    const auto key = makeNormalizedRowKey(
      sourceRow.objectAcronym,
      sourceRow.geometryType,
      sourceRow.tableName,
      sourceRow.sourceLookupId,
      sourceRow.sourceRcid);
    const auto compiledIt = compiledRows.find(key);
    const auto *compiledRow = compiledIt == compiledRows.end() ? nullptr : compiledIt->second;

    RowAnalysis rowAnalysis;
    rowAnalysis.objectAcronym = sourceRow.objectAcronym;
    rowAnalysis.geometryType = geometryTypeText(sourceRow.geometryType);
    rowAnalysis.tableName = sourceRow.tableName;
    rowAnalysis.sourceLookupId = sourceRow.sourceLookupId;
    rowAnalysis.sourceRcid = sourceRow.sourceRcid;
    rowAnalysis.ruleId = compiledRow == nullptr ? std::string{} : compiledRow->ruleId;
    rowAnalysis.instructionTokens = sortedUnique(std::move(statementTokens));
    rowAnalysis.pointAssets = sortedUnique(std::move(pointAssets));
    rowAnalysis.lineAssets = sortedUnique(std::move(lineAssets));
    rowAnalysis.areaAssets = sortedUnique(std::move(areaAssets));
    rowAnalysis.areaColorTokens = sortedUnique(std::move(areaColorTokens));
    rowAnalysis.textAssets = sortedUnique(std::move(textAssets));
    rowAnalysis.textAttributeKeys = sortedUnique(std::move(textAttributeKeys));
    rowAnalysis.conditionalTokens = sortedUnique(std::move(conditionalTokens));

    for(const auto &statement : parsedInstructions.unsupportedStatements) {
      const auto opcode = extractOpcode(statement);
      if(!opcode.empty()) {
        rowAnalysis.unsupportedInstructionTokens.push_back(opcode);
      }
    }
    rowAnalysis.unsupportedInstructionTokens = sortedUnique(std::move(rowAnalysis.unsupportedInstructionTokens));

    for(const auto &conditionId : rowAnalysis.conditionalTokens) {
      if(parseConditionalOpcode(conditionId) == S52ConditionalOpcode::kUnknown) {
        rowAnalysis.unresolvedConditionalTokens.push_back(conditionId);
      }
    }
    rowAnalysis.unresolvedConditionalTokens = sortedUnique(std::move(rowAnalysis.unresolvedConditionalTokens));

    rowAnalysis.harnessCovered =
      (!rowAnalysis.ruleId.empty() && coveredRuleIds.contains(rowAnalysis.ruleId))
      || (!rowAnalysis.sourceRcid.empty() && coveredSourceRcids.contains(rowAnalysis.sourceRcid));

    if(compiledRow == nullptr) {
      rowAnalysis.reasons.push_back("compiler_missing_lookup_row");
    } else if(!sourceRow.rawInstruction.empty() && compiledRow->instructions.empty()) {
      rowAnalysis.reasons.push_back("compiler_no_typed_instructions");
    }

    for(const auto &token : rowAnalysis.unsupportedInstructionTokens) {
      rowAnalysis.reasons.push_back("parser_unsupported:" + token);
    }

    for(const auto &conditionId : rowAnalysis.unresolvedConditionalTokens) {
      rowAnalysis.reasons.push_back("csp_unimplemented:" + conditionId);
    }

    for(const auto &assetId : rowAnalysis.pointAssets) {
      if(!containsAssetId(compiledCatalog.pointSymbols, assetId)) {
        rowAnalysis.reasons.push_back("compiler_missing_point_asset:" + assetId);
      }
    }
    for(const auto &assetId : rowAnalysis.lineAssets) {
      if(!containsAssetId(compiledCatalog.lineStyles, assetId)) {
        rowAnalysis.reasons.push_back("compiler_missing_line_asset:" + assetId);
      }
    }
    for(const auto &assetId : rowAnalysis.areaAssets) {
      if(!containsAssetId(compiledCatalog.areaPatterns, assetId)) {
        rowAnalysis.reasons.push_back("compiler_missing_area_asset:" + assetId);
      }
    }
    for(const auto &colorToken : rowAnalysis.areaColorTokens) {
      if(!std::any_of(
           compiledCatalog.colors.begin(),
           compiledCatalog.colors.end(),
           [&](const auto &color) { return color.token == colorToken; })) {
        rowAnalysis.reasons.push_back("compiler_missing_color_token:" + colorToken);
      }
    }
    for(const auto &assetId : rowAnalysis.textAssets) {
      if(assetId.empty()) {
        rowAnalysis.reasons.push_back("compiler_missing_text_asset");
      }
    }
    if(!rowAnalysis.harnessCovered) {
      rowAnalysis.reasons.push_back("scene_harness_not_covered");
    }

    rowAnalysis.reasons = sortedUnique(std::move(rowAnalysis.reasons));

    if(compiledRow == nullptr || (!sourceRow.rawInstruction.empty() && compiledRow != nullptr
                                  && compiledRow->instructions.empty())) {
      rowAnalysis.status = CoverageStatus::kUnsupported;
      ++result.unsupportedRows;
    } else if(!rowAnalysis.reasons.empty()) {
      rowAnalysis.status = CoverageStatus::kPartial;
      ++result.partialRows;
    } else {
      rowAnalysis.status = CoverageStatus::kSupported;
      ++result.supportedRows;
    }

    if(rowAnalysis.status != CoverageStatus::kSupported) {
      degradedRows.push_back(std::move(rowAnalysis));
    }
  }

  std::sort(
    degradedRows.begin(),
    degradedRows.end(),
    [](const RowAnalysis &lhs, const RowAnalysis &rhs) {
      return std::tie(
               lhs.objectAcronym,
               lhs.geometryType,
               lhs.tableName,
               lhs.sourceRcid,
               lhs.sourceLookupId)
           < std::tie(
               rhs.objectAcronym,
               rhs.geometryType,
               rhs.tableName,
               rhs.sourceRcid,
               rhs.sourceLookupId);
    });

  QJsonObject root;
  root.insert("schema", "s52_resource_snapshot_inventory_v1");
  root.insert("ok", true);

  QJsonObject snapshot;
  snapshot.insert("bundleRef", "Release_5.14.0");
  snapshot.insert("catalogId", QString::fromStdString(compiledCatalog.catalogId));
  snapshot.insert("bundleRoot", QString::fromStdWString(sourceBundleRoot.wstring()));
  snapshot.insert("chartsymbolsSha256", QString::fromStdString(sha256OfFile(sourceBundleRoot / "chartsymbols.xml")));
  snapshot.insert("lookupRowsTotal", result.lookupRowsTotal);
  snapshot.insert("colorsTotal", static_cast<int>(parseResult.catalog.colors.size()));
  snapshot.insert("pointSymbolsTotal", static_cast<int>(parseResult.catalog.pointSymbols.size()));
  snapshot.insert("lineStylesTotal", static_cast<int>(parseResult.catalog.lineStyles.size()));
  snapshot.insert("areaPatternsTotal", static_cast<int>(parseResult.catalog.areaPatterns.size()));
  root.insert("snapshot", snapshot);

  QJsonObject rowCoverage;
  rowCoverage.insert("supportedRows", result.supportedRows);
  rowCoverage.insert("partialRows", result.partialRows);
  rowCoverage.insert("unsupportedRows", result.unsupportedRows);
  rowCoverage.insert("degradedRows", static_cast<int>(degradedRows.size()));
  root.insert("rowCoverage", rowCoverage);

  QJsonObject distributions;
  distributions.insert("objects", stringCountObject(objectCounts, "objectAcronym"));
  distributions.insert("geometryTypes", stringCountObject(geometryCounts, "geometryType"));
  distributions.insert("tableNames", stringCountObject(tableCounts, "tableName"));
  distributions.insert("instructionTokens", stringCountObject(statementTokenCounts, "token"));
  distributions.insert("conditionalTokens", stringCountObject(conditionalTokenCounts, "token"));
  distributions.insert("pointAssetReferences", stringCountObject(pointAssetCounts, "assetId"));
  distributions.insert("lineAssetReferences", stringCountObject(lineAssetCounts, "assetId"));
  distributions.insert("areaAssetReferences", stringCountObject(areaAssetCounts, "assetId"));
  distributions.insert("areaColorReferences", stringCountObject(areaColorTokenCounts, "colorToken"));
  distributions.insert("textAssetReferences", stringCountObject(textAssetCounts, "assetId"));
  distributions.insert("textAttributeReferences", stringCountObject(textAttributeCounts, "attributeKey"));
  root.insert("distributions", distributions);

  QJsonObject parserCompilerSupport;
  parserCompilerSupport.insert("instructionTokens", tokenSupportObject(statementTokenStats));
  parserCompilerSupport.insert("conditionalTokens", conditionalSupportObject(conditionalTokenStats));
  root.insert("parserCompilerSupport", parserCompilerSupport);

  QJsonObject harnessCoverage;
  harnessCoverage.insert("referenceScenes", stringArray(sceneIds));
  harnessCoverage.insert("coveredRuleIds", static_cast<int>(coveredRuleIds.size()));
  harnessCoverage.insert("coveredSourceRcids", static_cast<int>(coveredSourceRcids.size()));
  root.insert("harnessCoverage", harnessCoverage);

  QJsonArray degradedRowsArray;
  for(const auto &row : degradedRows) {
    QJsonObject rowObject;
    rowObject.insert("status", QString::fromStdString(statusText(row.status)));
    rowObject.insert("objectAcronym", QString::fromStdString(row.objectAcronym));
    rowObject.insert("geometryType", QString::fromStdString(row.geometryType));
    rowObject.insert("tableName", QString::fromStdString(row.tableName));
    rowObject.insert("sourceLookupId", QString::fromStdString(row.sourceLookupId));
    rowObject.insert("sourceRcid", QString::fromStdString(row.sourceRcid));
    rowObject.insert("ruleId", QString::fromStdString(row.ruleId));
    rowObject.insert("harnessCovered", row.harnessCovered);
    rowObject.insert("reasons", stringArray(row.reasons));
    rowObject.insert("instructionTokens", stringArray(row.instructionTokens));
    rowObject.insert("unsupportedInstructionTokens", stringArray(row.unsupportedInstructionTokens));
    rowObject.insert("conditionalTokens", stringArray(row.conditionalTokens));
    rowObject.insert("unresolvedConditionalTokens", stringArray(row.unresolvedConditionalTokens));
    rowObject.insert("pointAssets", stringArray(row.pointAssets));
    rowObject.insert("lineAssets", stringArray(row.lineAssets));
    rowObject.insert("areaAssets", stringArray(row.areaAssets));
    rowObject.insert("areaColorTokens", stringArray(row.areaColorTokens));
    rowObject.insert("textAssets", stringArray(row.textAssets));
    rowObject.insert("textAttributeKeys", stringArray(row.textAttributeKeys));
    degradedRowsArray.push_back(rowObject);
  }
  root.insert("degradedRows", degradedRowsArray);

  result.document = QJsonDocument(root);
  return result;
}

} // namespace chart_view::test_support
