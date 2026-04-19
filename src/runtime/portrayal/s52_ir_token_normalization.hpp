#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S52_IR_TOKEN_NORMALIZATION_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S52_IR_TOKEN_NORMALIZATION_HPP

#include "s52_instruction_ir.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>
#include <unordered_set>

namespace chart_view::runtime::portrayal {

struct S52AssetIdNormalizationContext
{
  std::unordered_set<std::string> pointAssetIds;
  std::unordered_set<std::string> lineAssetIds;
  std::unordered_set<std::string> areaAssetIds;
};

[[nodiscard]] inline std::string normalizeIrToken(std::string_view value)
{
  std::string normalized;
  normalized.reserve(value.size());
  for(const auto ch : value) {
    if(std::isalnum(static_cast<unsigned char>(ch)) != 0) {
      normalized.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    } else if(ch == '_' || ch == '-' || ch == '/' || ch == '.') {
      normalized.push_back('_');
    }
  }
  return normalized;
}

[[nodiscard]] inline bool hasNormalizedAssetId(const std::unordered_set<std::string> &assetIds,
                                               std::string_view assetId)
{
  return assetIds.contains(std::string(assetId));
}

[[nodiscard]] inline bool looksLikeTextSpilloverSuffix(std::string_view suffix)
{
  return suffix.find("OBJNAM") != std::string_view::npos || suffix.find("NOBJNM") != std::string_view::npos
      || suffix.find("CHBLK") != std::string_view::npos || suffix.find("CHGRD") != std::string_view::npos;
}

[[nodiscard]] inline std::string normalizePointSymbolAssetId(std::string_view assetId,
                                                             const S52AssetIdNormalizationContext &context)
{
  const auto normalized = normalizeIrToken(assetId);
  if(normalized.empty()) {
    return normalized;
  }

  if(hasNormalizedAssetId(context.pointAssetIds, normalized)) {
    return normalized;
  }

  static constexpr std::array kExplicitAliases{
    std::pair<std::string_view, std::string_view>{"DGPS01DRFSTA01", "RDOSTA02"},
  };
  for(const auto &[legacyToken, canonicalToken] : kExplicitAliases) {
    if(normalized == legacyToken && hasNormalizedAssetId(context.pointAssetIds, canonicalToken)) {
      return std::string(canonicalToken);
    }
  }

  static constexpr std::array kSpilloverMarkers{
    std::string_view{"TESOBJNAM"},
    std::string_view{"TXOBJNAM"},
  };
  for(const auto marker : kSpilloverMarkers) {
    const auto markerPos = normalized.find(marker);
    if(markerPos == std::string::npos || markerPos == 0) {
      continue;
    }

    const auto candidate = normalized.substr(0, markerPos);
    const auto suffix = normalized.substr(markerPos + marker.size());
    if(looksLikeTextSpilloverSuffix(suffix) && hasNormalizedAssetId(context.pointAssetIds, candidate)) {
      return candidate;
    }
  }

  return normalized;
}

[[nodiscard]] inline std::string normalizeInstructionAssetIdForIr(
  std::string_view assetId,
  S52InstructionType type,
  const S52AssetIdNormalizationContext &context)
{
  switch(type) {
  case S52InstructionType::kPointSymbol:
    return normalizePointSymbolAssetId(assetId, context);
  case S52InstructionType::kLineStyle:
    return normalizeIrToken(assetId);
  case S52InstructionType::kAreaPattern:
    return normalizeIrToken(assetId);
  case S52InstructionType::kAreaColor:
    return normalizeIrToken(assetId);
  case S52InstructionType::kTextLabel:
    return normalizeIrToken(assetId);
  case S52InstructionType::kConditional:
    return normalizeIrToken(assetId);
  }

  return normalizeIrToken(assetId);
}

} // namespace chart_view::runtime::portrayal

#endif
