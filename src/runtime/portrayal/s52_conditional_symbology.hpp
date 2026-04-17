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
      result.suppressed = true;
      return result;
    }

    if(settings.honorScamin && exceedsScamin(feature, settings.viewingScaleDenominator)) {
      result.suppressed = true;
      return result;
    }

    executeCompiledConditionalOpcodes(feature, settings, result);

    if(!settings.showTextLabels) {
      std::erase_if(
        result.instructions,
        [](const S52Instruction &instruction) {
          return instructionType(instruction) == S52InstructionType::kTextLabel;
        });
    }

    if(result.lookupKey == "SOUNDG" && !settings.showSoundings) {
      std::erase_if(
        result.instructions,
        [](const S52Instruction &instruction) {
          return instructionType(instruction) == S52InstructionType::kTextLabel;
        });
      result.suppressed = true;
      return result;
    }

    if(result.lookupKey == "DEPARE"
       && !hasConditionalInstruction(result, S52ConditionalOpcode::kDepare01)
       && !hasConditionalInstruction(result, S52ConditionalOpcode::kDepare02)) {
      applyDepthConditionOutputs(feature, settings, result);
    }

    if(result.lookupKey == "LIGHTS"
       && settings.fullSectorLights
       && !hasConditionalInstruction(result, S52ConditionalOpcode::kLights05)) {
      appendConditionalInstruction(result, S52ConditionalOpcode::kFullSectorLights);
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
    S52ConditionalOpcode opcode)
  {
    const auto conditionId = conditionalOpcodeToken(opcode);
    const auto exists = std::any_of(
      result.instructions.begin(),
      result.instructions.end(),
      [&](const S52Instruction &instruction) {
        const auto *conditional = std::get_if<S52ConditionalInstruction>(&instruction);
        return conditional != nullptr && (conditional->opcode == opcode || conditional->conditionId == conditionId);
      });
    if(!exists) {
      result.instructions.push_back(makeConditionalInstruction(conditionId));
    }
  }

  [[nodiscard]] static bool hasConditionalInstruction(
    const S52LookupResult &result,
    S52ConditionalOpcode opcode) noexcept
  {
    return std::any_of(
      result.instructions.begin(),
      result.instructions.end(),
      [&](const S52Instruction &instruction) {
        const auto *conditional = std::get_if<S52ConditionalInstruction>(&instruction);
        return conditional != nullptr && conditional->opcode == opcode;
      });
  }

  static void executeCompiledConditionalOpcodes(
    const chart_data::Feature &feature,
    const S52DisplaySettings &settings,
    S52LookupResult &result)
  {
    const auto compiledInstructions = result.instructions;
    for(const auto &instruction : compiledInstructions) {
      const auto *conditional = std::get_if<S52ConditionalInstruction>(&instruction);
      if(conditional == nullptr) {
        continue;
      }

      switch(conditional->opcode) {
      case S52ConditionalOpcode::kLights05:
        if(settings.fullSectorLights) {
          appendConditionalInstruction(result, S52ConditionalOpcode::kFullSectorLights);
        }
        break;
      case S52ConditionalOpcode::kDepare01:
      case S52ConditionalOpcode::kDepare02:
        applyDepthConditionOutputs(feature, settings, result);
        break;
      case S52ConditionalOpcode::kUnknown:
      case S52ConditionalOpcode::kRestrn01:
      case S52ConditionalOpcode::kTopmar01:
      case S52ConditionalOpcode::kSlcons03:
      case S52ConditionalOpcode::kObstrn04:
      case S52ConditionalOpcode::kResare02:
      case S52ConditionalOpcode::kSymins01:
      case S52ConditionalOpcode::kDatcvr01:
      case S52ConditionalOpcode::kWrecks02:
      case S52ConditionalOpcode::kQuapos01:
      case S52ConditionalOpcode::kDepcnt02:
      case S52ConditionalOpcode::kSoundg02:
      case S52ConditionalOpcode::kOwnshp02:
      case S52ConditionalOpcode::kVessel01:
      case S52ConditionalOpcode::kClrlin01:
      case S52ConditionalOpcode::kLeglin02:
      case S52ConditionalOpcode::kPastrk01:
      case S52ConditionalOpcode::kResare01:
      case S52ConditionalOpcode::kTopmari1:
      case S52ConditionalOpcode::kVrmebl01:
      case S52ConditionalOpcode::kFullSectorLights:
      case S52ConditionalOpcode::kTwoShadesDepth:
      case S52ConditionalOpcode::kFullDepthShades:
      case S52ConditionalOpcode::kSymbolizedBoundaries:
      case S52ConditionalOpcode::kPlainBoundaries:
      case S52ConditionalOpcode::kShallowPattern:
      case S52ConditionalOpcode::kSafetyContourAlert:
        break;
      }
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
      appendConditionalInstruction(result, S52ConditionalOpcode::kTwoShadesDepth);
    } else {
      appendConditionalInstruction(result, S52ConditionalOpcode::kFullDepthShades);
    }

    if(settings.symbolizedBoundaries) {
      appendConditionalInstruction(result, S52ConditionalOpcode::kSymbolizedBoundaries);
    } else {
      appendConditionalInstruction(result, S52ConditionalOpcode::kPlainBoundaries);
    }

    if(settings.shallowPattern && minDepth.has_value() && *minDepth < settings.shallowContourMeters) {
      appendConditionalInstruction(result, S52ConditionalOpcode::kShallowPattern);
    }

    const auto relevantDepth = maxDepth.has_value() ? *maxDepth : (minDepth.has_value() ? *minDepth : -1.0);
    if(relevantDepth >= 0.0 && relevantDepth < settings.safetyContourMeters) {
      appendConditionalInstruction(result, S52ConditionalOpcode::kSafetyContourAlert);
    }
  }
};

}// namespace chart_view::runtime::portrayal

#endif
