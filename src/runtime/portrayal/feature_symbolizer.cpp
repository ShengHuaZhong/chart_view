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

std::string_view resolveSemanticStyleKey(const chart_data::Feature &feature) noexcept
{
  if(const auto styleKey = S57RuleTable::resolveStyleKey(feature); !styleKey.empty()) {
    return styleKey;
  }

  return S101RuleTable::resolveStyleKey(feature);
}

}// namespace

FeatureSymbolization FeatureSymbolizer::symbolize(const chart_data::Feature &feature) const
{
  FeatureSymbolization symbolization;
  symbolization.geometryType = chart_data::geometryType(feature.geometry);

  if(const auto s52Lookup =
       S52ConditionalSymbology::apply(feature, m_s52Settings, S52LookupModel::lookup(feature));
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
      case S52InstructionType::kTextLabel:
        if(symbolization.textKey.empty()) {
          symbolization.textKey = std::string(instructionStyleKey(instruction));
        }
        break;
      case S52InstructionType::kConditional:
        break;
      }
    }
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
  }

  return symbolization;
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
  return value != nullptr && !value->empty();
}

}// namespace chart_view::runtime::portrayal
