#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S52_CONDITIONAL_OPCODE_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S52_CONDITIONAL_OPCODE_HPP

#include <algorithm>
#include <cctype>
#include <iterator>
#include <string>
#include <string_view>

namespace chart_view::runtime::portrayal {

enum class S52ConditionalOpcode
{
  kUnknown,
  kRestrn01,
  kTopmar01,
  kSlcons03,
  kObstrn04,
  kResare02,
  kLights05,
  kSymins01,
  kDepare01,
  kDatcvr01,
  kWrecks02,
  kDepare02,
  kQuapos01,
  kDepcnt02,
  kSoundg02,
  kOwnshp02,
  kVessel01,
  kClrlin01,
  kLeglin02,
  kPastrk01,
  kResare01,
  kTopmari1,
  kVrmebl01,
  kFullSectorLights,
  kTwoShadesDepth,
  kFullDepthShades,
  kSymbolizedBoundaries,
  kPlainBoundaries,
  kShallowPattern,
  kSafetyContourAlert,
};

[[nodiscard]] inline std::string normalizeConditionalToken(std::string_view token)
{
  std::string normalized;
  normalized.reserve(token.size());
  std::transform(
    token.begin(),
    token.end(),
    std::back_inserter(normalized),
    [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
  return normalized;
}

[[nodiscard]] inline std::string_view canonicalConditionalOpcodeToken(S52ConditionalOpcode opcode) noexcept
{
  switch(opcode) {
  case S52ConditionalOpcode::kSlcons03:
    return "SLCONS04";
  case S52ConditionalOpcode::kObstrn04:
    return "OBSTRN07";
  case S52ConditionalOpcode::kResare02:
    return "RESARE04";
  case S52ConditionalOpcode::kLights05:
    return "LIGHTS06";
  case S52ConditionalOpcode::kSymins01:
    return "SYMINS02";
  case S52ConditionalOpcode::kWrecks02:
    return "WRECKS05";
  case S52ConditionalOpcode::kDepare02:
    return "DEPARE03";
  case S52ConditionalOpcode::kSoundg02:
    return "SOUNDG03";
  default:
    break;
  }

  return {};
}

[[nodiscard]] inline S52ConditionalOpcode parseConditionalOpcode(std::string_view token) noexcept
{
  const auto normalized = normalizeConditionalToken(token);
  if(normalized == "RESTRN01") {
    return S52ConditionalOpcode::kRestrn01;
  }
  if(normalized == "TOPMAR01") {
    return S52ConditionalOpcode::kTopmar01;
  }
  if(normalized == "SLCONS03") {
    return S52ConditionalOpcode::kSlcons03;
  }
  if(normalized == "SLCONS04") {
    return S52ConditionalOpcode::kSlcons03;
  }
  if(normalized == "OBSTRN04") {
    return S52ConditionalOpcode::kObstrn04;
  }
  if(normalized == "OBSTRN07") {
    return S52ConditionalOpcode::kObstrn04;
  }
  if(normalized == "RESARE02") {
    return S52ConditionalOpcode::kResare02;
  }
  if(normalized == "RESARE04") {
    return S52ConditionalOpcode::kResare02;
  }
  if(normalized == "LIGHTS05") {
    return S52ConditionalOpcode::kLights05;
  }
  if(normalized == "LIGHTS06") {
    return S52ConditionalOpcode::kLights05;
  }
  if(normalized == "SYMINS01") {
    return S52ConditionalOpcode::kSymins01;
  }
  if(normalized == "SYMINS02") {
    return S52ConditionalOpcode::kSymins01;
  }
  if(normalized == "DEPARE01") {
    return S52ConditionalOpcode::kDepare01;
  }
  if(normalized == "DATCVR01") {
    return S52ConditionalOpcode::kDatcvr01;
  }
  if(normalized == "WRECKS02") {
    return S52ConditionalOpcode::kWrecks02;
  }
  if(normalized == "WRECKS05") {
    return S52ConditionalOpcode::kWrecks02;
  }
  if(normalized == "DEPARE02") {
    return S52ConditionalOpcode::kDepare02;
  }
  if(normalized == "DEPARE03") {
    return S52ConditionalOpcode::kDepare02;
  }
  if(normalized == "QUAPOS01") {
    return S52ConditionalOpcode::kQuapos01;
  }
  if(normalized == "DEPCNT02") {
    return S52ConditionalOpcode::kDepcnt02;
  }
  if(normalized == "SOUNDG02") {
    return S52ConditionalOpcode::kSoundg02;
  }
  if(normalized == "SOUNDG03") {
    return S52ConditionalOpcode::kSoundg02;
  }
  if(normalized == "OWNSHP02") {
    return S52ConditionalOpcode::kOwnshp02;
  }
  if(normalized == "VESSEL01") {
    return S52ConditionalOpcode::kVessel01;
  }
  if(normalized == "CLRLIN01") {
    return S52ConditionalOpcode::kClrlin01;
  }
  if(normalized == "LEGLIN02") {
    return S52ConditionalOpcode::kLeglin02;
  }
  if(normalized == "PASTRK01") {
    return S52ConditionalOpcode::kPastrk01;
  }
  if(normalized == "RESARE01") {
    return S52ConditionalOpcode::kResare01;
  }
  if(normalized == "TOPMARI1") {
    return S52ConditionalOpcode::kTopmari1;
  }
  if(normalized == "VRMEBL01") {
    return S52ConditionalOpcode::kVrmebl01;
  }
  if(normalized == "FULL_SECTOR_LIGHTS") {
    return S52ConditionalOpcode::kFullSectorLights;
  }
  if(normalized == "TWO_SHADES_DEPTH") {
    return S52ConditionalOpcode::kTwoShadesDepth;
  }
  if(normalized == "FULL_DEPTH_SHADES") {
    return S52ConditionalOpcode::kFullDepthShades;
  }
  if(normalized == "SYMBOLIZED_BOUNDARIES") {
    return S52ConditionalOpcode::kSymbolizedBoundaries;
  }
  if(normalized == "PLAIN_BOUNDARIES") {
    return S52ConditionalOpcode::kPlainBoundaries;
  }
  if(normalized == "SHALLOW_PATTERN") {
    return S52ConditionalOpcode::kShallowPattern;
  }
  if(normalized == "SAFETY_CONTOUR_ALERT") {
    return S52ConditionalOpcode::kSafetyContourAlert;
  }
  return S52ConditionalOpcode::kUnknown;
}

[[nodiscard]] inline std::string_view conditionalOpcodeToken(S52ConditionalOpcode opcode) noexcept
{
  switch(opcode) {
  case S52ConditionalOpcode::kRestrn01:
    return "RESTRN01";
  case S52ConditionalOpcode::kTopmar01:
    return "TOPMAR01";
  case S52ConditionalOpcode::kSlcons03:
    return "SLCONS03";
  case S52ConditionalOpcode::kObstrn04:
    return "OBSTRN04";
  case S52ConditionalOpcode::kResare02:
    return "RESARE02";
  case S52ConditionalOpcode::kLights05:
    return "LIGHTS05";
  case S52ConditionalOpcode::kSymins01:
    return "SYMINS01";
  case S52ConditionalOpcode::kDepare01:
    return "DEPARE01";
  case S52ConditionalOpcode::kDatcvr01:
    return "DATCVR01";
  case S52ConditionalOpcode::kWrecks02:
    return "WRECKS02";
  case S52ConditionalOpcode::kDepare02:
    return "DEPARE02";
  case S52ConditionalOpcode::kQuapos01:
    return "QUAPOS01";
  case S52ConditionalOpcode::kDepcnt02:
    return "DEPCNT02";
  case S52ConditionalOpcode::kSoundg02:
    return "SOUNDG02";
  case S52ConditionalOpcode::kOwnshp02:
    return "OWNSHP02";
  case S52ConditionalOpcode::kVessel01:
    return "VESSEL01";
  case S52ConditionalOpcode::kClrlin01:
    return "CLRLIN01";
  case S52ConditionalOpcode::kLeglin02:
    return "LEGLIN02";
  case S52ConditionalOpcode::kPastrk01:
    return "PASTRK01";
  case S52ConditionalOpcode::kResare01:
    return "RESARE01";
  case S52ConditionalOpcode::kTopmari1:
    return "TOPMARI1";
  case S52ConditionalOpcode::kVrmebl01:
    return "VRMEBL01";
  case S52ConditionalOpcode::kFullSectorLights:
    return "full_sector_lights";
  case S52ConditionalOpcode::kTwoShadesDepth:
    return "two_shades_depth";
  case S52ConditionalOpcode::kFullDepthShades:
    return "full_depth_shades";
  case S52ConditionalOpcode::kSymbolizedBoundaries:
    return "symbolized_boundaries";
  case S52ConditionalOpcode::kPlainBoundaries:
    return "plain_boundaries";
  case S52ConditionalOpcode::kShallowPattern:
    return "shallow_pattern";
  case S52ConditionalOpcode::kSafetyContourAlert:
    return "safety_contour_alert";
  case S52ConditionalOpcode::kUnknown:
    return {};
  }

  return {};
}

} // namespace chart_view::runtime::portrayal

#endif
