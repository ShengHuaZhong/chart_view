#include "portrayal_registry.hpp"
#include "s52_presentation_assets.hpp"

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
  const S52PresentationAssets s52Assets;
  m_canvasBackgroundColor = s52Assets.resolveColor("NODTA", m_canvasBackgroundColor);

  registerSymbolRuleForStyle("point/default", m_defaultSymbolRule);
  registerLineStyleRuleForStyle("line/default", m_defaultLineStyleRule);
  registerAreaFillRuleForStyle("area/default", m_defaultAreaFillRule);

  const auto registerPointStyle =
    [this, &s52Assets](std::string_view styleKey,
                       std::string_view assetId,
                       const SymbolRule &fallback) {
      if(const auto *asset = s52Assets.findPointSymbol(assetId); asset != nullptr) {
        registerSymbolRuleForStyle(
          styleKey,
          {s52Assets.resolveColor(asset->colorToken, fallback.color), asset->radius});
        return;
      }

      registerSymbolRuleForStyle(styleKey, fallback);
    };

  const auto registerLineStyle =
    [this, &s52Assets](std::string_view styleKey,
                       std::string_view assetId,
                       const LineStyleRule &fallback) {
      if(const auto *asset = s52Assets.findLineStyle(assetId); asset != nullptr) {
        registerLineStyleRuleForStyle(
          styleKey,
          {s52Assets.resolveColor(asset->colorToken, fallback.color), asset->thickness});
        return;
      }

      registerLineStyleRuleForStyle(styleKey, fallback);
    };

  const auto registerAreaStyle =
    [this, &s52Assets](std::string_view styleKey,
                       std::string_view assetId,
                       const AreaFillRule &fallback) {
      if(const auto *asset = s52Assets.findAreaPattern(assetId); asset != nullptr) {
        auto fillColor = s52Assets.resolveColor(asset->fillColorToken, fallback.fillColor);
        fillColor[3] = asset->fillAlpha;
        registerAreaFillRuleForStyle(
          styleKey,
          {fillColor,
           s52Assets.resolveColor(asset->outlineColorToken, fallback.outlineColor),
           s52Assets.resolveColor(asset->holeFillColorToken, fallback.holeFillColor),
           asset->outlineThickness});
        return;
      }

      registerAreaFillRuleForStyle(styleKey, fallback);
    };

  registerPointStyle("point/sounding", "SOUNDG01", m_defaultSymbolRule);
  registerPointStyle("point/buoy", "BOYSPP01", {{220U, 176U, 32U, 255U}, 4});
  registerPointStyle("point/beacon", "BCNSPP01", {{110U, 96U, 52U, 255U}, 4});
  registerPointStyle("point/danger", "DANGER01", {{210U, 92U, 28U, 255U}, 4});
  registerPointStyle("point/landmark", "LNDMRK01", {{70U, 70U, 70U, 255U}, 4});

  registerLineStyle("line/depth_contour", "DEPCN01", m_defaultLineStyleRule);
  registerLineStyle("line/coastline", "COALNE01", m_defaultLineStyleRule);
  registerLineStyle("line/channel", "FAIRWY01", {{24U, 116U, 86U, 255U}, 2});

  registerAreaStyle("area/depth", "DEPARE01", m_defaultAreaFillRule);
  registerAreaStyle(
    "area/land",
    "LNDARE01",
    {{196U, 190U, 137U, 255U}, {110U, 96U, 52U, 255U}, {230U, 230U, 217U, 255U}, 1});
  registerAreaStyle(
    "area/restricted",
    "RESARE01",
    {{229U, 196U, 196U, 220U}, {160U, 58U, 58U, 255U}, {230U, 230U, 217U, 255U}, 1});

  registerTextRuleForStyle(
    "text/default",
    {s52Assets.resolveColor("CHBLK", m_defaultTextRule.color), m_defaultTextRule.pixelSize});
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
