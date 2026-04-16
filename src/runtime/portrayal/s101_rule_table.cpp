#include "s101_rule_table.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>

namespace chart_view::runtime::portrayal {

namespace {

struct S101RuleEntry
{
  std::string_view className;
  std::string_view styleKey;
};

constexpr std::array<S101RuleEntry, 14> kPointRules{{
  {"sounding", "point/sounding"},
  {"buoyspecialpurpose", "point/buoy"},
  {"buoylateral", "point/buoy"},
  {"buoysafewater", "point/buoy"},
  {"beaconspecialpurpose", "point/beacon"},
  {"beaconlateral", "point/beacon"},
  {"beaconsafewater", "point/beacon"},
  {"wreck", "point/danger"},
  {"underwaterrock", "point/danger"},
  {"light", "point/landmark"},
  {"landmark", "point/landmark"},
  {"pilotboardingplace", "point/landmark"},
  {"soundg", "point/sounding"},
  {"lndmrk", "point/landmark"},
}};

constexpr std::array<S101RuleEntry, 7> kLineRules{{
  {"depthcontour", "line/depth_contour"},
  {"coastline", "line/coastline"},
  {"fairway", "line/channel"},
  {"canal", "line/channel"},
  {"river", "line/channel"},
  {"depcnt", "line/depth_contour"},
  {"coalne", "line/coastline"},
}};

constexpr std::array<S101RuleEntry, 7> kAreaRules{{
  {"deptharea", "area/depth"},
  {"dredgedarea", "area/depth"},
  {"landarea", "area/land"},
  {"restrictedarea", "area/restricted"},
  {"unsurveyedarea", "area/restricted"},
  {"depare", "area/depth"},
  {"drgare", "area/depth"},
}};

template<std::size_t N>
std::string_view lookup(
  std::string_view className,
  const std::array<S101RuleEntry, N> &rules) noexcept
{
  const auto it = std::find_if(
    rules.begin(),
    rules.end(),
    [&](const auto &rule) { return rule.className == className; });
  return it == rules.end() ? std::string_view{} : it->styleKey;
}

std::string normalizeClassName(std::string_view value)
{
  std::string normalized;
  normalized.reserve(value.size());

  for(const auto ch : value) {
    if(ch == ' ' || ch == '_' || ch == '-') {
      continue;
    }

    normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }

  return normalized;
}

}// namespace

std::string_view S101RuleTable::resolveStyleKey(const chart_data::Feature &feature) noexcept
{
  const auto className = normalizeClassName(feature.classAcronym);
  if(className.empty()) {
    return {};
  }

  switch(chart_data::geometryType(feature.geometry)) {
  case chart_data::GeometryType::kPoint:
    return lookup(className, kPointRules);
  case chart_data::GeometryType::kLine:
    return lookup(className, kLineRules);
  case chart_data::GeometryType::kArea:
    return lookup(className, kAreaRules);
  }

  return {};
}

}// namespace chart_view::runtime::portrayal
