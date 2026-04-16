#include "portrayal_registry.hpp"

#include <algorithm>
#include <cctype>

namespace chart_view::runtime::portrayal {

namespace {

template<typename Rule>
const Rule &lookupRule(const std::unordered_map<std::string, Rule> &rules,
                       std::string_view featureAcronym,
                       const Rule &fallback)
{
  if(featureAcronym.empty()) {
    return fallback;
  }

  const auto key = PortrayalRegistry::normalizeKey(featureAcronym);
  const auto it = rules.find(key);
  return it == rules.end() ? fallback : it->second;
}

}// namespace

PortrayalRegistry::PortrayalRegistry()
{
  registerSymbolRuleForStyle("point/default", m_defaultSymbolRule);
  registerSymbolRuleForStyle("point/sounding", m_defaultSymbolRule);
  registerSymbolRuleForStyle("point/buoy", {{220U, 176U, 32U, 255U}, 4});
  registerSymbolRuleForStyle("point/beacon", {{180U, 84U, 30U, 255U}, 4});
  registerSymbolRuleForStyle("point/danger", {{210U, 92U, 28U, 255U}, 4});
  registerSymbolRuleForStyle("point/landmark", {{70U, 70U, 70U, 255U}, 4});

  registerLineStyleRuleForStyle("line/default", m_defaultLineStyleRule);
  registerLineStyleRuleForStyle("line/depth_contour", m_defaultLineStyleRule);
  registerLineStyleRuleForStyle("line/coastline", m_defaultLineStyleRule);
  registerLineStyleRuleForStyle("line/channel", {{24U, 116U, 86U, 255U}, 2});

  registerAreaFillRuleForStyle("area/default", m_defaultAreaFillRule);
  registerAreaFillRuleForStyle("area/depth", m_defaultAreaFillRule);
  registerAreaFillRuleForStyle(
    "area/land",
    {{196U, 190U, 137U, 255U}, {110U, 96U, 52U, 255U}, {230U, 230U, 217U, 255U}, 1});
  registerAreaFillRuleForStyle(
    "area/restricted",
    {{229U, 196U, 196U, 220U}, {160U, 58U, 58U, 255U}, {230U, 230U, 217U, 255U}, 1});

  registerTextRuleForStyle("text/default", m_defaultTextRule);
}

std::string PortrayalRegistry::normalizeKey(std::string_view value)
{
  std::string normalized(value);
  std::transform(
    normalized.begin(),
    normalized.end(),
    normalized.begin(),
    [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return normalized;
}

void PortrayalRegistry::registerSymbolRule(std::string_view featureAcronym, SymbolRule rule)
{
  if(featureAcronym.empty()) {
    return;
  }

  m_symbolRules[normalizeKey(featureAcronym)] = rule;
}

void PortrayalRegistry::registerLineStyleRule(std::string_view featureAcronym, LineStyleRule rule)
{
  if(featureAcronym.empty()) {
    return;
  }

  m_lineRules[normalizeKey(featureAcronym)] = rule;
}

void PortrayalRegistry::registerAreaFillRule(std::string_view featureAcronym, AreaFillRule rule)
{
  if(featureAcronym.empty()) {
    return;
  }

  m_areaRules[normalizeKey(featureAcronym)] = rule;
}

void PortrayalRegistry::registerTextRule(std::string_view featureAcronym, TextRule rule)
{
  if(featureAcronym.empty()) {
    return;
  }

  m_textRules[normalizeKey(featureAcronym)] = rule;
}

void PortrayalRegistry::registerSymbolRuleForStyle(std::string_view styleKey, SymbolRule rule)
{
  if(styleKey.empty()) {
    return;
  }

  m_symbolStyleRules[normalizeKey(styleKey)] = rule;
}

void PortrayalRegistry::registerLineStyleRuleForStyle(std::string_view styleKey, LineStyleRule rule)
{
  if(styleKey.empty()) {
    return;
  }

  m_lineStyleRules[normalizeKey(styleKey)] = rule;
}

void PortrayalRegistry::registerAreaFillRuleForStyle(std::string_view styleKey, AreaFillRule rule)
{
  if(styleKey.empty()) {
    return;
  }

  m_areaStyleRules[normalizeKey(styleKey)] = rule;
}

void PortrayalRegistry::registerTextRuleForStyle(std::string_view styleKey, TextRule rule)
{
  if(styleKey.empty()) {
    return;
  }

  m_textStyleRules[normalizeKey(styleKey)] = rule;
}

const SymbolRule &PortrayalRegistry::resolveSymbolRule(
  const chart_data::Feature &feature) const
{
  return lookupRule(m_symbolRules, feature.classAcronym, m_defaultSymbolRule);
}

const LineStyleRule &PortrayalRegistry::resolveLineStyleRule(
  const chart_data::Feature &feature) const
{
  return lookupRule(m_lineRules, feature.classAcronym, m_defaultLineStyleRule);
}

const AreaFillRule &PortrayalRegistry::resolveAreaFillRule(
  const chart_data::Feature &feature) const
{
  return lookupRule(m_areaRules, feature.classAcronym, m_defaultAreaFillRule);
}

const TextRule &PortrayalRegistry::resolveTextRule(
  const chart_data::Feature &feature) const
{
  return lookupRule(m_textRules, feature.classAcronym, m_defaultTextRule);
}

const SymbolRule &PortrayalRegistry::resolveSymbolRuleForStyle(std::string_view styleKey) const
{
  return lookupRule(m_symbolStyleRules, styleKey, m_defaultSymbolRule);
}

const LineStyleRule &PortrayalRegistry::resolveLineStyleRuleForStyle(std::string_view styleKey) const
{
  return lookupRule(m_lineStyleRules, styleKey, m_defaultLineStyleRule);
}

const AreaFillRule &PortrayalRegistry::resolveAreaFillRuleForStyle(std::string_view styleKey) const
{
  return lookupRule(m_areaStyleRules, styleKey, m_defaultAreaFillRule);
}

const TextRule &PortrayalRegistry::resolveTextRuleForStyle(std::string_view styleKey) const
{
  return lookupRule(m_textStyleRules, styleKey, m_defaultTextRule);
}

}// namespace chart_view::runtime::portrayal
