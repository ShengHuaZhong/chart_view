#ifndef CHART_VIEW_RUNTIME_S57_S57_SEMANTIC_MAPPING_HPP
#define CHART_VIEW_RUNTIME_S57_S57_SEMANTIC_MAPPING_HPP

#include <array>
#include <cstdint>
#include <string_view>

namespace chart_view::runtime::s57 {

namespace detail {

struct CodeMapping
{
  std::uint16_t code{0};
  std::string_view acronym;
};

template<std::size_t N>
constexpr std::string_view lookupCode(
  std::uint16_t code,
  const std::array<CodeMapping, N> &mappings) noexcept
{
  for(const auto &mapping : mappings) {
    if(mapping.code == code) {
      return mapping.acronym;
    }
  }

  return {};
}

}// namespace detail

inline constexpr std::array<detail::CodeMapping, 22> kS57ObjectClassMappings{{
  {7, "BCNLAT"},
  {8, "BCNSAW"},
  {9, "BCNSPP"},
  {17, "BOYLAT"},
  {18, "BOYSAW"},
  {19, "BOYSPP"},
  {23, "CANALS"},
  {30, "COALNE"},
  {42, "DEPARE"},
  {43, "DEPCNT"},
  {46, "DRGARE"},
  {51, "FAIRWY"},
  {71, "LNDARE"},
  {74, "LNDMRK"},
  {75, "LIGHTS"},
  {90, "PILPNT"},
  {112, "RESARE"},
  {114, "RIVERS"},
  {129, "SOUNDG"},
  {153, "UWTROC"},
  {154, "UNSARE"},
  {159, "WRECKS"},
}};

inline constexpr std::array<detail::CodeMapping, 7> kS57AttributeMappings{{
  {15, "CATCOA"},
  {87, "DRVAL1"},
  {88, "DRVAL2"},
  {116, "OBJNAM"},
  {174, "VALDCO"},
  {179, "VALSOU"},
  {301, "NOBJNM"},
}};

inline constexpr std::string_view lookupObjectClassAcronym(std::uint16_t code) noexcept
{
  return detail::lookupCode(code, kS57ObjectClassMappings);
}

inline constexpr std::string_view lookupAttributeAcronym(std::uint16_t code) noexcept
{
  return detail::lookupCode(code, kS57AttributeMappings);
}

}// namespace chart_view::runtime::s57

#endif
