#include "feature_symbolizer.hpp"
#include "s101_rule_table.hpp"
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

  if(hasNonEmptyStringAttribute(feature, "OBJNAM") || hasNonEmptyStringAttribute(feature, "NOBJNM")) {
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
