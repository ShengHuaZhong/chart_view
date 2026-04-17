#include "feature_symbolizer.hpp"
#include "s101_rule_table.hpp"
#include "s52_conditional_symbology.hpp"
#include "s52_lookup_model.hpp"
#include "s57_rule_table.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace chart_view::runtime::portrayal {

namespace {

std::string toUpperAscii(std::string_view value)
{
  std::string result(value);
  std::transform(
    result.begin(),
    result.end(),
    result.begin(),
    [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
  return result;
}

std::string toLowerAscii(std::string_view value)
{
  std::string result(value);
  std::transform(
    result.begin(),
    result.end(),
    result.begin(),
    [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return result;
}

std::string_view resolveSemanticStyleKey(const chart_data::Feature &feature) noexcept
{
  if(const auto styleKey = S57RuleTable::resolveStyleKey(feature); !styleKey.empty()) {
    return styleKey;
  }

  return S101RuleTable::resolveStyleKey(feature);
}

std::string compiledTextAttributeKey(const std::optional<S52LookupResult> &lookup)
{
  if(!lookup.has_value()) {
    return {};
  }

  for(const auto &instruction : lookup->instructions) {
    if(instructionType(instruction) != S52InstructionType::kTextLabel) {
      continue;
    }

    const auto attributeKey = instructionTextAttributeKey(instruction);
    if(!attributeKey.empty()) {
      return std::string(attributeKey);
    }
  }

  return {};
}

bool hasInstructionAsset(const std::optional<S52LookupResult> &lookup)
{
  if(!lookup.has_value()) {
    return false;
  }

  return std::any_of(
    lookup->instructions.begin(),
    lookup->instructions.end(),
    [](const S52Instruction &instruction) { return !instructionAssetId(instruction).empty(); });
}

std::string preferredSourceTextAttributeKey(const chart_data::Feature &feature)
{
  if(const auto it = feature.attributes.find("NOBJNM"); it != feature.attributes.end()) {
    if(const auto *value = std::get_if<std::string>(&it->second); value != nullptr && !value->empty()) {
      return "NOBJNM";
    }
    if(const auto *values = std::get_if<chart_view::runtime::chart_data::AttributeStringList>(&it->second);
       values != nullptr
       && std::any_of(values->begin(), values->end(), [](const std::string &entry) { return !entry.empty(); })) {
      return "NOBJNM";
    }
  }

  if(const auto it = feature.attributes.find("OBJNAM"); it != feature.attributes.end()) {
    if(const auto *value = std::get_if<std::string>(&it->second); value != nullptr && !value->empty()) {
      return "OBJNAM";
    }
    if(const auto *values = std::get_if<chart_view::runtime::chart_data::AttributeStringList>(&it->second);
       values != nullptr
       && std::any_of(values->begin(), values->end(), [](const std::string &entry) { return !entry.empty(); })) {
      return "OBJNAM";
    }
  }

  return {};
}

}// namespace

FeatureSymbolization FeatureSymbolizer::symbolize(const chart_data::Feature &feature) const
{
  FeatureSymbolization symbolization;
  symbolization.geometryType = chart_data::geometryType(feature.geometry);

  const auto ungatedS52Lookup = S52LookupModel::lookup(feature, m_s52Settings);
  const auto compiledTextAttribute = compiledTextAttributeKey(ungatedS52Lookup);

  if(const auto s52Lookup =
       S52ConditionalSymbology::apply(feature, m_s52Settings, ungatedS52Lookup);
     s52Lookup.has_value()) {
    symbolization.s52Lookup = s52Lookup;
    symbolization.suppressed = s52Lookup->suppressed;
    for(const auto &instruction : s52Lookup->instructions) {
      switch(instructionType(instruction)) {
      case S52InstructionType::kPointSymbol:
      case S52InstructionType::kLineStyle:
      case S52InstructionType::kAreaPattern:
        if(symbolization.styleKey.empty()) {
          symbolization.styleKey = std::string(instructionStyleKey(instruction));
        }
        break;
      case S52InstructionType::kAreaColor:
        break;
      case S52InstructionType::kTextLabel:
        if(symbolization.textKey.empty()) {
          const auto attributeKey = instructionTextAttributeKey(instruction);
          symbolization.textKey = std::string(instructionStyleKey(instruction));
          if(!attributeKey.empty()) {
            symbolization.textAttributeKey = std::string(attributeKey);
          }
        }
        break;
      case S52InstructionType::kConditional:
        break;
      }
    }
  }

  if(!symbolization.suppressed
     && hasInstructionAsset(symbolization.s52Lookup)
     && symbolization.textAttributeKey.empty()
     && !compiledTextAttribute.empty()) {
    symbolization.textAttributeKey = compiledTextAttribute;
  }

  if(!symbolization.suppressed
     && hasInstructionAsset(symbolization.s52Lookup)
     && symbolization.textAttributeKey.empty()
     && !m_s52Settings.showTextLabels) {
    symbolization.textAttributeKey = preferredSourceTextAttributeKey(feature);
  }

  if(!isObjectClassEnabled(feature.classAcronym)) {
    symbolization.suppressed = true;
  }

  if(symbolization.s52Lookup.has_value() && !isRuleEnabled(symbolization.s52Lookup->ruleId)) {
    symbolization.suppressed = true;
  }

  if(symbolization.styleKey.empty() && !symbolization.suppressed) {
    switch(symbolization.geometryType) {
    case chart_data::GeometryType::kPoint:
      if(const auto styleKey = resolveSemanticStyleKey(feature); !styleKey.empty()) {
        symbolization.styleKey = std::string(styleKey);
      } else if(hasAttribute(feature, "VALSOU")) {
        symbolization.styleKey = "point/sounding";
      } else if(hasClassPrefix(feature, "BOY")) {
        symbolization.styleKey = "point/buoy";
      } else if(hasClassPrefix(feature, "BCN")) {
        symbolization.styleKey = "point/beacon";
      } else {
        symbolization.styleKey = "point/default";
      }
      break;

    case chart_data::GeometryType::kLine:
      if(const auto styleKey = resolveSemanticStyleKey(feature); !styleKey.empty()) {
        symbolization.styleKey = std::string(styleKey);
      } else if(hasAttribute(feature, "VALDCO")) {
        symbolization.styleKey = "line/depth_contour";
      } else if(hasAttribute(feature, "CATCOA")) {
        symbolization.styleKey = "line/coastline";
      } else {
        symbolization.styleKey = "line/default";
      }
      break;

    case chart_data::GeometryType::kArea:
      if(const auto styleKey = resolveSemanticStyleKey(feature); !styleKey.empty()) {
        symbolization.styleKey = std::string(styleKey);
      } else if(hasAttribute(feature, "DRVAL1") || hasAttribute(feature, "DRVAL2")) {
        symbolization.styleKey = "area/depth";
      } else {
        symbolization.styleKey = "area/default";
      }
      break;
    }
  }

  if(symbolization.textKey.empty()
     && m_s52Settings.showTextLabels
     && !symbolization.suppressed
     && (hasNonEmptyStringAttribute(feature, "OBJNAM") || hasNonEmptyStringAttribute(feature, "NOBJNM"))) {
    symbolization.textKey = "text/default";
    symbolization.textAttributeKey =
      hasNonEmptyStringAttribute(feature, "NOBJNM") ? "NOBJNM" : "OBJNAM";
  }

  return symbolization;
}

void FeatureSymbolizer::setS57ClassFilters(std::span<const S57ClassSelectionFilter> filters)
{
  m_s57ClassFilters.assign(filters.begin(), filters.end());
  for(auto &filter : m_s57ClassFilters) {
    filter.objectAcronym = toUpperAscii(filter.objectAcronym);
  }
}

void FeatureSymbolizer::setS52RuleFilters(std::span<const S52RuleSelectionFilter> filters)
{
  m_s52RuleFilters.assign(filters.begin(), filters.end());
  for(auto &filter : m_s52RuleFilters) {
    filter.ruleId = toLowerAscii(filter.ruleId);
  }
}

bool FeatureSymbolizer::hasAttribute(
  const chart_data::Feature &feature,
  std::string_view key) noexcept
{
  return feature.attributes.contains(std::string(key));
}

bool FeatureSymbolizer::hasClassPrefix(
  const chart_data::Feature &feature,
  std::string_view prefix) noexcept
{
  if(feature.classAcronym.size() < prefix.size()) {
    return false;
  }

  return std::equal(
    prefix.begin(),
    prefix.end(),
    feature.classAcronym.begin(),
    [](char lhs, char rhs) {
      return std::toupper(static_cast<unsigned char>(lhs))
          == std::toupper(static_cast<unsigned char>(rhs));
    });
}

bool FeatureSymbolizer::hasNonEmptyStringAttribute(
  const chart_data::Feature &feature,
  std::string_view key) noexcept
{
  const auto it = feature.attributes.find(std::string(key));
  if(it == feature.attributes.end()) {
    return false;
  }

  const auto *value = std::get_if<std::string>(&it->second);
  if(value != nullptr) {
    return !value->empty();
  }

  const auto *values = std::get_if<chart_view::runtime::chart_data::AttributeStringList>(&it->second);
  return values != nullptr
      && std::any_of(values->begin(), values->end(), [](const std::string &entry) { return !entry.empty(); });
}

bool FeatureSymbolizer::isObjectClassEnabled(std::string_view objectAcronym) const noexcept
{
  if(m_s57ClassFilters.empty()) {
    return true;
  }

  const auto normalized = toUpperAscii(objectAcronym);
  const auto it = std::find_if(
    m_s57ClassFilters.begin(),
    m_s57ClassFilters.end(),
    [&](const S57ClassSelectionFilter &filter) { return filter.objectAcronym == normalized; });
  return it == m_s57ClassFilters.end() ? true : it->enabled;
}

bool FeatureSymbolizer::isRuleEnabled(std::string_view ruleId) const noexcept
{
  if(m_s52RuleFilters.empty() || ruleId.empty()) {
    return true;
  }

  const auto normalized = toLowerAscii(ruleId);
  const auto it = std::find_if(
    m_s52RuleFilters.begin(),
    m_s52RuleFilters.end(),
    [&](const S52RuleSelectionFilter &filter) { return filter.ruleId == normalized; });
  return it == m_s52RuleFilters.end() ? true : it->enabled;
}

}// namespace chart_view::runtime::portrayal
