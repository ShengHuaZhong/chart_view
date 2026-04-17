#include "s52_lookup_model.hpp"

#include "s52_source_catalog_compiler.hpp"
#include "s57_rule_table.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <optional>
#include <type_traits>
#include <tuple>

namespace chart_view::runtime::portrayal {

namespace {

const S52CompiledCatalog &compiledCatalog()
{
  static const auto catalog = S52SourceCatalogCompiler::compilePreferred();
  return catalog;
}

std::string normalizeAcronymValue(std::string_view value)
{
  std::string normalized(value);
  std::transform(
    normalized.begin(),
    normalized.end(),
    normalized.begin(),
    [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
  return normalized;
}

bool hasNonEmptyTextAttribute(const chart_data::Feature &feature, std::string_view key)
{
  const auto it = feature.attributes.find(std::string(key));
  if(it == feature.attributes.end()) {
    return false;
  }

  if(const auto *value = std::get_if<std::string>(&it->second)) {
    return !value->empty();
  }

  if(const auto *values = std::get_if<chart_data::AttributeStringList>(&it->second)) {
    return !values->empty() && std::any_of(values->begin(), values->end(), [](const auto &entry) {
      return !entry.empty();
    });
  }

  return false;
}

bool featureHasAttribute(const chart_data::Feature &feature, std::string_view key)
{
  return feature.attributes.contains(std::string(key));
}

std::string semanticStyleKey(const chart_data::Feature &feature)
{
  if(const auto styleKey = S57RuleTable::resolveStyleKey(feature); !styleKey.empty()) {
    return std::string(styleKey);
  }

  return {};
}

std::string trimUpperAscii(std::string_view value)
{
  while(!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
    value.remove_prefix(1);
  }

  while(!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
    value.remove_suffix(1);
  }

  return normalizeAcronymValue(value);
}

struct AttributeRequirement
{
  enum class Kind
  {
    kUnsupported,
    kPresence,
    kExactNumeric,
  };

  Kind kind{Kind::kUnsupported};
  std::string key;
  std::int64_t exactNumericValue{0};
};

AttributeRequirement parseAttributeRequirement(std::string_view token)
{
  auto normalized = trimUpperAscii(token);
  if(normalized.empty() || normalized.front() == '$') {
    return {};
  }

  if(normalized.back() == '?') {
    normalized.pop_back();
    if(normalized.empty()) {
      return {};
    }

    return {AttributeRequirement::Kind::kPresence, std::move(normalized), 0};
  }

  std::size_t digitStart = normalized.size();
  while(digitStart > 0
        && std::isdigit(static_cast<unsigned char>(normalized[digitStart - 1])) != 0) {
    --digitStart;
  }

  if(digitStart == normalized.size()) {
    return {AttributeRequirement::Kind::kPresence, std::move(normalized), 0};
  }

  if(digitStart == 0) {
    return {};
  }

  const auto key = normalized.substr(0, digitStart);
  const auto numericSuffix = normalized.substr(digitStart);
  try {
    return {
      AttributeRequirement::Kind::kExactNumeric,
      key,
      std::stoll(numericSuffix)};
  } catch(...) {
    return {};
  }
}

bool doubleMatches(std::int64_t expected, double value) noexcept
{
  return std::isfinite(value)
      && std::fabs(value - static_cast<double>(expected)) < 1e-9;
}

bool attributeMatchesNumeric(const chart_data::AttributeValue &value, std::int64_t expected)
{
  if(const auto *intValue = std::get_if<std::int64_t>(&value)) {
    return *intValue == expected;
  }

  if(const auto *doubleValue = std::get_if<double>(&value)) {
    return doubleMatches(expected, *doubleValue);
  }

  if(const auto *stringValue = std::get_if<std::string>(&value)) {
    return trimUpperAscii(*stringValue) == std::to_string(expected);
  }

  if(const auto *intValues = std::get_if<chart_data::AttributeIntList>(&value)) {
    return std::find(intValues->begin(), intValues->end(), expected) != intValues->end();
  }

  if(const auto *doubleValues = std::get_if<chart_data::AttributeDoubleList>(&value)) {
    return std::any_of(
      doubleValues->begin(),
      doubleValues->end(),
      [&](double candidate) { return doubleMatches(expected, candidate); });
  }

  if(const auto *stringValues = std::get_if<chart_data::AttributeStringList>(&value)) {
    const auto expectedText = std::to_string(expected);
    return std::any_of(
      stringValues->begin(),
      stringValues->end(),
      [&](const std::string &candidate) { return trimUpperAscii(candidate) == expectedText; });
  }

  return false;
}

struct RowMatchScore
{
  bool compatible{true};
  int matchedRequirements{0};
  int supportedRequirements{0};
  int unsupportedRequirements{0};
  int geometryInstructionCount{0};
  int nonConditionalInstructionCount{0};
};

int tablePreferenceScore(
  const S52CompiledLookupRow &row,
  chart_data::GeometryType geometryType,
  const S52DisplaySettings &settings) noexcept
{
  const auto normalizedTable = normalizeAcronymValue(row.tableName);
  if(geometryType == chart_data::GeometryType::kPoint) {
    if(settings.pointSymbolMode == S52PointSymbolMode::kTraditional) {
      if(normalizedTable == "PAPER") {
        return 4;
      }
      if(normalizedTable == "SIMPLIFIED") {
        return 1;
      }
    } else {
      if(normalizedTable == "SIMPLIFIED") {
        return 4;
      }
      if(normalizedTable == "PAPER") {
        return 1;
      }
    }
  }

  if(geometryType == chart_data::GeometryType::kArea || geometryType == chart_data::GeometryType::kLine) {
    if(geometryType == chart_data::GeometryType::kLine && normalizedTable == "LINES") {
      return 4;
    }
    if(settings.symbolizedBoundaries && normalizedTable == "SYMBOLIZED") {
      return 3;
    }
    if(!settings.symbolizedBoundaries && normalizedTable == "PLAIN") {
      return 3;
    }
    if(normalizedTable == "SYMBOLIZED" || normalizedTable == "PLAIN") {
      return 2;
    }
  }

  return normalizedTable.empty() ? 0 : 1;
}

RowMatchScore evaluateAttributeCodes(const chart_data::Feature &feature, const S52CompiledLookupRow &row)
{
  RowMatchScore score;
  for(const auto &instruction : row.instructions) {
    switch(instructionType(instruction)) {
    case S52InstructionType::kPointSymbol:
    case S52InstructionType::kLineStyle:
    case S52InstructionType::kAreaPattern:
    case S52InstructionType::kAreaColor:
      ++score.geometryInstructionCount;
      ++score.nonConditionalInstructionCount;
      break;
    case S52InstructionType::kTextLabel:
      ++score.nonConditionalInstructionCount;
      break;
    case S52InstructionType::kConditional:
      break;
    }
  }

  for(const auto &attributeCode : row.attributeCodes) {
    const auto requirement = parseAttributeRequirement(attributeCode);
    if(requirement.kind == AttributeRequirement::Kind::kUnsupported || requirement.key.empty()) {
      ++score.unsupportedRequirements;
      continue;
    }

    ++score.supportedRequirements;
    const auto it = feature.attributes.find(requirement.key);
    if(it == feature.attributes.end()) {
      score.compatible = false;
      return score;
    }

    bool matches = false;
    switch(requirement.kind) {
    case AttributeRequirement::Kind::kPresence:
      matches = featureHasAttribute(feature, requirement.key);
      break;
    case AttributeRequirement::Kind::kExactNumeric:
      matches = attributeMatchesNumeric(it->second, requirement.exactNumericValue);
      break;
    case AttributeRequirement::Kind::kUnsupported:
      break;
    }

    if(!matches) {
      score.compatible = false;
      return score;
    }

    ++score.matchedRequirements;
  }

  return score;
}

std::string preferredTextAttributeKey(const chart_data::Feature &feature)
{
  if(hasNonEmptyTextAttribute(feature, "NOBJNM")) {
    return "NOBJNM";
  }
  if(hasNonEmptyTextAttribute(feature, "OBJNAM")) {
    return "OBJNAM";
  }
  return {};
}

const S52CompiledLookupRow *findCompiledRow(
  const chart_data::Feature &feature,
  const S52DisplaySettings &settings)
{
  const auto normalizedAcronym = normalizeAcronymValue(feature.classAcronym);
  if(normalizedAcronym.empty()) {
    return nullptr;
  }

  const auto geometryType = chart_data::geometryType(feature.geometry);
  const auto &catalog = compiledCatalog();
  const S52CompiledLookupRow *bestRow = nullptr;
  RowMatchScore bestScore{};

  for(const auto &row : catalog.lookupRows) {
    if(row.objectAcronym != normalizedAcronym || row.geometryType != geometryType
       || row.instructions.empty()) {
      continue;
    }

    const auto score = evaluateAttributeCodes(feature, row);
    if(!score.compatible) {
      continue;
    }

    if(bestRow == nullptr) {
      bestRow = &row;
      bestScore = score;
      continue;
    }

    const auto candidateRank = std::make_tuple(
      tablePreferenceScore(row, geometryType, settings),
      score.matchedRequirements,
      score.supportedRequirements,
      score.geometryInstructionCount,
      score.nonConditionalInstructionCount,
      row.instructionFallback ? 0 : 1,
      -score.unsupportedRequirements,
      !row.sourceRcid.empty() ? 1 : 0,
      !row.sourceLookupId.empty() ? 1 : 0,
      row.ruleId);
    const auto bestRank = std::make_tuple(
      tablePreferenceScore(*bestRow, geometryType, settings),
      bestScore.matchedRequirements,
      bestScore.supportedRequirements,
      bestScore.geometryInstructionCount,
      bestScore.nonConditionalInstructionCount,
      bestRow->instructionFallback ? 0 : 1,
      -bestScore.unsupportedRequirements,
      !bestRow->sourceRcid.empty() ? 1 : 0,
      !bestRow->sourceLookupId.empty() ? 1 : 0,
      bestRow->ruleId);
    if(candidateRank > bestRank) {
      bestRow = &row;
      bestScore = score;
    }
  }

  return bestRow;
}

bool hasTextInstruction(const S52LookupResult &result) noexcept
{
  return std::any_of(
    result.instructions.begin(),
    result.instructions.end(),
    [](const S52Instruction &instruction) {
      return instructionType(instruction) == S52InstructionType::kTextLabel;
    });
}

void normalizeConditionalOpcodes(S52LookupResult &result)
{
  for(auto &instruction : result.instructions) {
    auto *conditionalInstruction = std::get_if<S52ConditionalInstruction>(&instruction);
    if(conditionalInstruction == nullptr) {
      continue;
    }

    conditionalInstruction->opcode = parseConditionalOpcode(conditionalInstruction->conditionId);
  }
}

void applySemanticStyleFallback(
  const chart_data::Feature &feature,
  S52LookupResult &result)
{
  const auto fallbackStyleKey = semanticStyleKey(feature);
  if(fallbackStyleKey.empty()) {
    return;
  }

  for(auto &instruction : result.instructions) {
    std::visit(
      [&](auto &typedInstruction) {
        using T = std::decay_t<decltype(typedInstruction)>;
        if constexpr(
          std::is_same_v<T, S52PointSymbolInstruction> || std::is_same_v<T, S52LineStyleInstruction>
          || std::is_same_v<T, S52AreaPatternInstruction>) {
          if(typedInstruction.styleKey.empty()) {
            typedInstruction.styleKey = fallbackStyleKey;
          }
        }
      },
      instruction);
  }
}

} // namespace

std::optional<S52LookupResult> S52LookupModel::lookup(
  const chart_data::Feature &feature,
  const S52DisplaySettings &settings)
{
  const auto *row = findCompiledRow(feature, settings);
  if(row == nullptr) {
    return std::nullopt;
  }

  S52LookupResult result;
  result.lookupKey = row->objectAcronym;
  result.ruleId = row->ruleId;
  result.displayCategory = row->displayCategory;
  result.displayPriority = row->displayPriority;
  result.viewGroup = row->viewGroup;
  result.instructions = row->instructions;
  result.sourceLookupId = row->sourceLookupId;
  result.sourceRcid = row->sourceRcid;
  result.tableName = row->tableName;
  result.attributeCodes = row->attributeCodes;
  result.rawInstruction = row->rawInstruction;
  result.instructionFallback = row->instructionFallback;
  normalizeConditionalOpcodes(result);
  applySemanticStyleFallback(feature, result);

  if(const auto attributeKey = preferredTextAttributeKey(feature);
     !attributeKey.empty() && !hasTextInstruction(result)) {
    result.instructions.push_back(S52TextInstruction{"TEXT01", "text/default", attributeKey});
  }

  return result;
}

std::string S52LookupModel::normalizeAcronym(std::string_view value)
{
  return normalizeAcronymValue(value);
}

} // namespace chart_view::runtime::portrayal
