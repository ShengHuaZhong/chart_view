#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S52_CONDITIONAL_SYMBOLOGY_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S52_CONDITIONAL_SYMBOLOGY_HPP

#include "s52_display_settings.hpp"
#include "s52_lookup_model.hpp"

#include "../chart_data/feature.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string_view>

namespace chart_view::runtime::portrayal {

class S52ConditionalSymbology
{
public:
  [[nodiscard]] static std::optional<S52LookupResult> apply(
    const chart_data::Feature &feature,
    const S52DisplaySettings &settings,
    std::optional<S52LookupResult> lookup)
  {
    if(!lookup.has_value()) {
      return std::nullopt;
    }

    auto result = *lookup;

    if(!displayCategoryVisible(result.displayCategory, settings.displayCategory)) {
      result.instructions.clear();
      result.suppressed = true;
      return result;
    }

    if(settings.honorScamin && exceedsScamin(feature, settings.viewingScaleDenominator)) {
      result.instructions.clear();
      result.suppressed = true;
      return result;
    }

    if(result.lookupKey == "SOUNDG" && !settings.showSoundings) {
      result.instructions.clear();
      result.suppressed = true;
      return result;
    }

    if(!settings.showTextLabels) {
      std::erase_if(
        result.instructions,
        [](const S52Instruction &instruction) {
          return instructionType(instruction) == S52InstructionType::kTextLabel;
        });
    }

    if(settings.pointSymbolMode == S52PointSymbolMode::kSimplified) {
      for(auto &instruction : result.instructions) {
        auto *pointInstruction = std::get_if<S52PointSymbolInstruction>(&instruction);
        if(pointInstruction == nullptr) {
          continue;
        }

        if(pointInstruction->styleKey == "point/buoy") {
          pointInstruction->assetId = "BOYSPP02";
        } else if(pointInstruction->styleKey == "point/beacon") {
          pointInstruction->assetId = "BCNSPP02";
        }
      }
    }

    if(result.lookupKey == "DEPARE") {
      applyDepthConditionOutputs(feature, settings, result);
    }

    if(result.lookupKey == "LIGHTS" && settings.fullSectorLights) {
      result.instructions.push_back(S52ConditionalInstruction{"full_sector_lights"});
    }

    return result;
  }

private:
  [[nodiscard]] static bool displayCategoryVisible(
    std::string_view ruleDisplayCategory,
    S52DisplayCategory activeCategory) noexcept
  {
    if(ruleDisplayCategory == "all") {
      return activeCategory == S52DisplayCategory::kAll;
    }

    if(ruleDisplayCategory == "display_base" || ruleDisplayCategory == "displaybase"
       || ruleDisplayCategory == "base") {
      return true;
    }

    return activeCategory != S52DisplayCategory::kDisplayBase;
  }

  [[nodiscard]] static std::optional<double> numericAttribute(
    const chart_data::Feature &feature,
    std::string_view key) noexcept
  {
    const auto it = feature.attributes.find(std::string(key));
    if(it == feature.attributes.end()) {
      return std::nullopt;
    }

    if(const auto *value = std::get_if<double>(&it->second)) {
      return *value;
    }

    if(const auto *value = std::get_if<std::int64_t>(&it->second)) {
      return static_cast<double>(*value);
    }

    return std::nullopt;
  }

  [[nodiscard]] static bool exceedsScamin(
    const chart_data::Feature &feature,
    double viewingScaleDenominator) noexcept
  {
    if(viewingScaleDenominator <= 0.0 || !std::isfinite(viewingScaleDenominator)) {
      return false;
    }

    const auto scamin = numericAttribute(feature, "SCAMIN");
    return scamin.has_value() && *scamin > 0.0 && viewingScaleDenominator > *scamin;
  }

  static void appendConditionalInstruction(
    S52LookupResult &result,
    std::string_view conditionId)
  {
    const auto exists = std::any_of(
      result.instructions.begin(),
      result.instructions.end(),
      [&](const S52Instruction &instruction) {
        const auto *conditional = std::get_if<S52ConditionalInstruction>(&instruction);
        return conditional != nullptr && conditional->conditionId == conditionId;
      });
    if(!exists) {
      result.instructions.push_back(S52ConditionalInstruction{std::string(conditionId)});
    }
  }

  static void applyDepthConditionOutputs(
    const chart_data::Feature &feature,
    const S52DisplaySettings &settings,
    S52LookupResult &result)
  {
    const auto minDepth = numericAttribute(feature, "DRVAL1");
    const auto maxDepth = numericAttribute(feature, "DRVAL2");

    if(settings.twoShades) {
      appendConditionalInstruction(result, "two_shades_depth");
    } else {
      appendConditionalInstruction(result, "full_depth_shades");
    }

    if(settings.symbolizedBoundaries) {
      appendConditionalInstruction(result, "symbolized_boundaries");
    } else {
      appendConditionalInstruction(result, "plain_boundaries");
    }

    if(settings.shallowPattern && minDepth.has_value() && *minDepth < settings.shallowContourMeters) {
      appendConditionalInstruction(result, "shallow_pattern");
    }

    const auto relevantDepth = maxDepth.has_value() ? *maxDepth : (minDepth.has_value() ? *minDepth : -1.0);
    if(relevantDepth >= 0.0 && relevantDepth < settings.safetyContourMeters) {
      appendConditionalInstruction(result, "safety_contour_alert");
    }
  }
};

}// namespace chart_view::runtime::portrayal

#endif
