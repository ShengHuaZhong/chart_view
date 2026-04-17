#include "portrayal_registry.hpp"
#include "s52_presentation_assets.hpp"

#include <algorithm>
#include <cctype>

namespace chart_view::runtime::portrayal {

namespace {

S52PaletteId paletteForSettings(const S52DisplaySettings &settings) noexcept
{
  switch(settings.colorScheme) {
  case S52ColorScheme::kDusk:
    return S52PaletteId::kDusk;
  case S52ColorScheme::kNight:
    return S52PaletteId::kNight;
  case S52ColorScheme::kDay:
  default:
    return S52PaletteId::kDay;
  }
}

const S52PresentationAssets &cachedPresentationAssets(S52PaletteId palette)
{
  // Keep these palette catalogs alive for the lifetime of the process. Debug CRT
  // teardown across DLL boundaries has been unreliable for these large cached
  // asset maps, while they are immutable and intentionally process-global.
  static const auto *dayAssets = new S52PresentationAssets(S52PaletteId::kDay);
  static const auto *duskAssets = new S52PresentationAssets(S52PaletteId::kDusk);
  static const auto *nightAssets = new S52PresentationAssets(S52PaletteId::kNight);

  switch(palette) {
  case S52PaletteId::kDusk:
    return *duskAssets;
  case S52PaletteId::kNight:
    return *nightAssets;
  case S52PaletteId::kDay:
  default:
    return *dayAssets;
  }
}

SurfaceColor withAlpha(SurfaceColor color, std::uint8_t alpha) noexcept
{
  color[3] = alpha;
  return color;
}

AreaFillRule makeAreaFillRule(
  const S52PresentationAssets &assets,
  std::string_view fillToken,
  std::string_view outlineToken,
  std::string_view holeFillToken,
  int outlineThickness,
  std::uint8_t fillAlpha)
{
  auto rule = AreaFillRule{};
  rule.fillColor = withAlpha(assets.resolveColor(fillToken, rule.fillColor), fillAlpha);
  rule.outlineColor = assets.resolveColor(outlineToken, rule.outlineColor);
  rule.holeFillColor = assets.resolveColor(holeFillToken, rule.holeFillColor);
  rule.outlineThickness = outlineThickness;
  return rule;
}

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
  : PortrayalRegistry(S52DisplaySettings{})
{
}

PortrayalRegistry::PortrayalRegistry(const S52DisplaySettings &settings)
{
  const auto &s52Assets = cachedPresentationAssets(paletteForSettings(settings));
  m_canvasBackgroundColor = s52Assets.resolveColor("NODTA", m_canvasBackgroundColor);
  m_defaultLineStyleRule.color = s52Assets.resolveColor("CHBLK", m_defaultLineStyleRule.color);
  m_defaultAreaFillRule.fillColor =
    withAlpha(s52Assets.resolveColor("DEPDW", m_defaultAreaFillRule.fillColor), m_defaultAreaFillRule.fillColor[3]);
  m_defaultAreaFillRule.outlineColor = s52Assets.resolveColor("DEPSC", m_defaultAreaFillRule.outlineColor);
  m_defaultAreaFillRule.holeFillColor = s52Assets.resolveColor("NODTA", m_defaultAreaFillRule.holeFillColor);
  m_defaultTextRule.color = s52Assets.resolveColor("CHBLK", m_defaultTextRule.color);

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
  registerPointStyle("point/light_sector", "LNDMRK01", {{176U, 68U, 22U, 255U}, 4});

  registerLineStyle("line/depth_contour", "DEPCN01", m_defaultLineStyleRule);
  registerLineStyle("line/coastline", "COALNE01", m_defaultLineStyleRule);
  registerLineStyle("line/channel", "FAIRWY01", {{24U, 116U, 86U, 255U}, 2});

  registerAreaStyle("area/depth", "DEPARE01", makeAreaFillRule(s52Assets, "DEPDW", "DEPSC", "NODTA", 1, 204U));
  registerAreaStyle(
    "area/depth_safety_alert",
    "DEPARE01",
    makeAreaFillRule(s52Assets, "DEPMD", "CHRED", "NODTA", 2, 220U));
  registerAreaStyle(
    "area/depth_shallow_pattern",
    "DEPARE01",
    makeAreaFillRule(s52Assets, "DEPMS", "CHGRN", "NODTA", 2, 204U));
  registerAreaStyle(
    "area/depth_symbolized_boundary",
    "DEPARE01",
    makeAreaFillRule(s52Assets, "DEPDW", "CHGRN", "NODTA", 2, 204U));
  registerAreaStyle(
    "area/depth_plain_boundary",
    "DEPARE01",
    makeAreaFillRule(s52Assets, "DEPDW", "CHBLK", "NODTA", 1, 204U));
  registerAreaStyle(
    "area/depth_full_shades",
    "DEPARE01",
    makeAreaFillRule(s52Assets, "DEPMD", "DEPSC", "NODTA", 1, 204U));
  registerAreaStyle(
    "area/depth_full_shades_symbolized_boundary",
    "DEPARE01",
    makeAreaFillRule(s52Assets, "DEPMD", "CHGRN", "NODTA", 2, 204U));
  registerAreaStyle(
    "area/depth_full_shades_plain_boundary",
    "DEPARE01",
    makeAreaFillRule(s52Assets, "DEPMD", "CHBLK", "NODTA", 1, 204U));
  registerAreaStyle(
    "area/depth_two_shades",
    "DEPARE01",
    makeAreaFillRule(s52Assets, "DEPIT", "DEPSC", "NODTA", 1, 204U));
  registerAreaStyle(
    "area/depth_two_shades_symbolized_boundary",
    "DEPARE01",
    makeAreaFillRule(s52Assets, "DEPIT", "CHGRN", "NODTA", 2, 204U));
  registerAreaStyle(
    "area/depth_two_shades_plain_boundary",
    "DEPARE01",
    makeAreaFillRule(s52Assets, "DEPIT", "CHBLK", "NODTA", 1, 204U));
  registerAreaStyle(
    "area/land",
    "LNDARE01",
    makeAreaFillRule(s52Assets, "LANDF", "CHBRN", "NODTA", 1, 255U));
  registerAreaStyle(
    "area/restricted",
    "RESARE01",
    makeAreaFillRule(s52Assets, "RESDR", "CHRED", "NODTA", 1, 220U));

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
