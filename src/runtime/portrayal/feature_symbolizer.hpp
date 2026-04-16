#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_FEATURE_SYMBOLIZER_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_FEATURE_SYMBOLIZER_HPP

#include "s52_display_settings.hpp"
#include "s52_lookup_model.hpp"

#include "../chart_data/feature.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace chart_view::runtime::portrayal {

struct FeatureSymbolization
{
  chart_data::GeometryType geometryType{chart_data::GeometryType::kPoint};
  std::string styleKey;
  std::string textKey;
  std::optional<S52LookupResult> s52Lookup;
  bool suppressed{false};
};

struct S57ClassSelectionFilter
{
  std::string objectAcronym;
  bool enabled{true};
};

struct S52RuleSelectionFilter
{
  std::string ruleId;
  bool enabled{true};
};

class FeatureSymbolizer
{
public:
  explicit FeatureSymbolizer(S52DisplaySettings settings = {})
    : m_s52Settings(settings)
  {
  }

  [[nodiscard]] FeatureSymbolization symbolize(const chart_data::Feature &feature) const;
  void setS52Settings(const S52DisplaySettings &settings) noexcept { m_s52Settings = settings; }
  [[nodiscard]] const S52DisplaySettings &s52Settings() const noexcept { return m_s52Settings; }
  void setS57ClassFilters(std::span<const S57ClassSelectionFilter> filters);
  void setS52RuleFilters(std::span<const S52RuleSelectionFilter> filters);

private:
  [[nodiscard]] static bool hasClassPrefix(
    const chart_data::Feature &feature,
    std::string_view prefix) noexcept;
  [[nodiscard]] static bool hasAttribute(
    const chart_data::Feature &feature,
    std::string_view key) noexcept;
  [[nodiscard]] static bool hasNonEmptyStringAttribute(
    const chart_data::Feature &feature,
    std::string_view key) noexcept;
  [[nodiscard]] bool isObjectClassEnabled(std::string_view objectAcronym) const noexcept;
  [[nodiscard]] bool isRuleEnabled(std::string_view ruleId) const noexcept;

  S52DisplaySettings m_s52Settings;
  std::vector<S57ClassSelectionFilter> m_s57ClassFilters;
  std::vector<S52RuleSelectionFilter> m_s52RuleFilters;
};

}// namespace chart_view::runtime::portrayal

#endif
