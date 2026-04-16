#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S101_RULE_TABLE_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S101_RULE_TABLE_HPP

#include "../chart_data/feature.hpp"

#include <string_view>

namespace chart_view::runtime::portrayal {

class S101RuleTable
{
public:
  [[nodiscard]] static std::string_view resolveStyleKey(const chart_data::Feature &feature) noexcept;
};

}// namespace chart_view::runtime::portrayal

#endif
