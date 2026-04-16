#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_FEATURE_SYMBOLIZER_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_FEATURE_SYMBOLIZER_HPP

#include "s52_display_settings.hpp"
#include "s52_lookup_model.hpp"

#include "../chart_data/feature.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace chart_view::runtime::portrayal {

struct FeatureSymbolization
{
  chart_data::GeometryType geometryType{chart_data::GeometryType::kPoint};
  std::string styleKey;
  std::string textKey;
  std::optional<S52LookupResult> s52Lookup;
  bool suppressed{false};
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

  S52DisplaySettings m_s52Settings;
};

}// namespace chart_view::runtime::portrayal

#endif
