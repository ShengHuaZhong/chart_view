#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S52_CONDITIONAL_SYMBOLOGY_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S52_CONDITIONAL_SYMBOLOGY_HPP

#include "s52_display_settings.hpp"
#include "s52_lookup_model.hpp"

#include "../chart_data/feature.hpp"

#include <algorithm>
#include <optional>

namespace chart_view::runtime::portrayal {

class S52ConditionalSymbology
{
public:
  [[nodiscard]] static std::optional<S52LookupResult> apply(
    const chart_data::Feature & /*feature*/,
    const S52DisplaySettings &settings,
    std::optional<S52LookupResult> lookup)
  {
    if(!lookup.has_value()) {
      return std::nullopt;
    }

    auto result = *lookup;

    if(result.lookupKey == "SOUNDG" && !settings.showSoundings) {
      result.instructions.clear();
      result.suppressed = true;
      return result;
    }

    if(!settings.showTextLabels) {
      std::erase_if(
        result.instructions,
        [](const S52Instruction &instruction) {
          return instruction.type == S52InstructionType::kTextLabel;
        });
    }

    if(settings.pointSymbolMode == S52PointSymbolMode::kSimplified) {
      for(auto &instruction : result.instructions) {
        if(instruction.type != S52InstructionType::kPointSymbol) {
          continue;
        }

        if(instruction.styleKey == "point/buoy") {
          instruction.assetId = "BOYSPP02";
        } else if(instruction.styleKey == "point/beacon") {
          instruction.assetId = "BCNSPP02";
        }
      }
    }

    return result;
  }
};

}// namespace chart_view::runtime::portrayal

#endif
