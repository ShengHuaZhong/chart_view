#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_PORTRAYAL_REGISTRY_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_PORTRAYAL_REGISTRY_HPP

#include "s52_display_settings.hpp"

#include "../chart_data/feature.hpp"
#include "../render_types.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace chart_view::runtime::portrayal {

struct SymbolRule
{
  SurfaceColor color{196U, 46U, 46U, 255U};
  int radius{4};
};

struct LineStyleRule
{
  SurfaceColor color{24U, 38U, 55U, 255U};
  int thickness{2};
};

struct AreaFillRule
{
  SurfaceColor fillColor{162U, 201U, 229U, 204U};
  SurfaceColor outlineColor{44U, 91U, 134U, 255U};
  SurfaceColor holeFillColor{230U, 230U, 217U, 255U};
  int outlineThickness{1};
};

struct TextRule
{
  SurfaceColor color{24U, 38U, 55U, 255U};
  std::uint32_t pixelSize{12U};
};

class PortrayalRegistry
{
public:
  PortrayalRegistry();
  explicit PortrayalRegistry(const S52DisplaySettings &settings);

  void setCanvasBackgroundColor(SurfaceColor color) noexcept { m_canvasBackgroundColor = color; }
  void setDefaultSymbolRule(SymbolRule rule) noexcept { m_defaultSymbolRule = rule; }
  void setDefaultLineStyleRule(LineStyleRule rule) noexcept { m_defaultLineStyleRule = rule; }
  void setDefaultAreaFillRule(AreaFillRule rule) noexcept { m_defaultAreaFillRule = rule; }
  void setDefaultTextRule(TextRule rule) noexcept { m_defaultTextRule = rule; }

  [[nodiscard]] const SurfaceColor &canvasBackgroundColor() const noexcept
  {
    return m_canvasBackgroundColor;
  }
  [[nodiscard]] const SymbolRule &defaultSymbolRule() const noexcept { return m_defaultSymbolRule; }
  [[nodiscard]] const LineStyleRule &defaultLineStyleRule() const noexcept
  {
    return m_defaultLineStyleRule;
  }
  [[nodiscard]] const AreaFillRule &defaultAreaFillRule() const noexcept
  {
    return m_defaultAreaFillRule;
  }
  [[nodiscard]] const TextRule &defaultTextRule() const noexcept { return m_defaultTextRule; }

  void registerSymbolRule(std::string_view featureAcronym, SymbolRule rule);
  void registerLineStyleRule(std::string_view featureAcronym, LineStyleRule rule);
  void registerAreaFillRule(std::string_view featureAcronym, AreaFillRule rule);
  void registerTextRule(std::string_view featureAcronym, TextRule rule);

  void registerSymbolRuleForStyle(std::string_view styleKey, SymbolRule rule);
  void registerLineStyleRuleForStyle(std::string_view styleKey, LineStyleRule rule);
  void registerAreaFillRuleForStyle(std::string_view styleKey, AreaFillRule rule);
  void registerTextRuleForStyle(std::string_view styleKey, TextRule rule);

  [[nodiscard]] static std::string normalizeKey(std::string_view value);

  [[nodiscard]] const SymbolRule &resolveSymbolRule(const chart_data::Feature &feature) const;
  [[nodiscard]] const LineStyleRule &resolveLineStyleRule(
    const chart_data::Feature &feature) const;
  [[nodiscard]] const AreaFillRule &resolveAreaFillRule(
    const chart_data::Feature &feature) const;
  [[nodiscard]] const TextRule &resolveTextRule(const chart_data::Feature &feature) const;

  [[nodiscard]] const SymbolRule &resolveSymbolRuleForStyle(std::string_view styleKey) const;
  [[nodiscard]] const LineStyleRule &resolveLineStyleRuleForStyle(std::string_view styleKey) const;
  [[nodiscard]] const AreaFillRule &resolveAreaFillRuleForStyle(std::string_view styleKey) const;
  [[nodiscard]] const TextRule &resolveTextRuleForStyle(std::string_view styleKey) const;

private:
  SurfaceColor m_canvasBackgroundColor{230U, 230U, 217U, 255U};
  SymbolRule m_defaultSymbolRule;
  LineStyleRule m_defaultLineStyleRule;
  AreaFillRule m_defaultAreaFillRule;
  TextRule m_defaultTextRule;
  std::unordered_map<std::string, SymbolRule> m_symbolRules;
  std::unordered_map<std::string, LineStyleRule> m_lineRules;
  std::unordered_map<std::string, AreaFillRule> m_areaRules;
  std::unordered_map<std::string, TextRule> m_textRules;
  std::unordered_map<std::string, SymbolRule> m_symbolStyleRules;
  std::unordered_map<std::string, LineStyleRule> m_lineStyleRules;
  std::unordered_map<std::string, AreaFillRule> m_areaStyleRules;
  std::unordered_map<std::string, TextRule> m_textStyleRules;
};

}// namespace chart_view::runtime::portrayal

#endif
