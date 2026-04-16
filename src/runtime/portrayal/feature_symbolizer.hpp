#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_FEATURE_SYMBOLIZER_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_FEATURE_SYMBOLIZER_HPP

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
};

class FeatureSymbolizer
{
public:
  FeatureSymbolizer() = default;

  [[nodiscard]] FeatureSymbolization symbolize(const chart_data::Feature &feature) const;

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
};

}// namespace chart_view::runtime::portrayal

#endif
