#include "s57_rule_table.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>

namespace chart_view::runtime::portrayal {

namespace {

struct S57RuleEntry
{
  std::string_view acronym;
  std::string_view styleKey;
};

constexpr std::array<S57RuleEntry, 12> kRules{{
  {"SOUNDG", "point/sounding"},
  {"BOYSPP", "point/buoy"},
  {"BOYLAT", "point/buoy"},
  {"BOYSAW", "point/buoy"},
  {"BCNSPP", "point/beacon"},
  {"BCNLAT", "point/beacon"},
  {"BCNSAW", "point/beacon"},
  {"WRECKS", "point/danger"},
  {"UWTROC", "point/danger"},
  {"LIGHTS", "point/landmark"},
  {"LNDMRK", "point/landmark"},
  {"PILPNT", "point/landmark"},
}};

constexpr std::array<S57RuleEntry, 5> kLineRules{{
  {"DEPCNT", "line/depth_contour"},
  {"COALNE", "line/coastline"},
  {"FAIRWY", "line/channel"},
  {"CANALS", "line/channel"},
  {"RIVERS", "line/channel"},
}};

constexpr std::array<S57RuleEntry, 5> kAreaRules{{
  {"DEPARE", "area/depth"},
  {"DRGARE", "area/depth"},
  {"LNDARE", "area/land"},
  {"RESARE", "area/restricted"},
  {"UNSARE", "area/restricted"},
}};

template<std::size_t N>
std::string_view lookup(
  std::string_view acronym,
  const std::array<S57RuleEntry, N> &rules) noexcept
{
  const auto it = std::find_if(
    rules.begin(),
    rules.end(),
    [&](const auto &rule) { return rule.acronym == acronym; });
  return it == rules.end() ? std::string_view{} : it->styleKey;
}

std::string normalizeAcronym(std::string_view value)
{
  std::string normalized(value);
  std::transform(
    normalized.begin(),
    normalized.end(),
    normalized.begin(),
    [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
  return normalized;
}

}// namespace

std::string_view S57RuleTable::resolveStyleKey(const chart_data::Feature &feature) noexcept
{
  const auto acronym = normalizeAcronym(feature.classAcronym);
  if(acronym.empty()) {
    return {};
  }

  switch(chart_data::geometryType(feature.geometry)) {
  case chart_data::GeometryType::kPoint:
    return lookup(acronym, kRules);
  case chart_data::GeometryType::kLine:
    return lookup(acronym, kLineRules);
  case chart_data::GeometryType::kArea:
    return lookup(acronym, kAreaRules);
  }

  return {};
}

}// namespace chart_view::runtime::portrayal
