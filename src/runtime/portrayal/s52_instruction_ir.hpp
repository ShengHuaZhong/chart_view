#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S52_INSTRUCTION_IR_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S52_INSTRUCTION_IR_HPP

#include "s52_conditional_opcode.hpp"

#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace chart_view::runtime::portrayal {

enum class S52InstructionType
{
  kPointSymbol,
  kLineStyle,
  kAreaPattern,
  kTextLabel,
  kConditional,
};

struct S52PointSymbolInstruction
{
  std::string assetId;
  std::string styleKey;
};

struct S52LineStyleInstruction
{
  std::string assetId;
  std::string styleKey;
};

struct S52AreaPatternInstruction
{
  std::string assetId;
  std::string styleKey;
};

struct S52TextInstruction
{
  std::string assetId;
  std::string styleKey;
  std::string attributeKey;
};

struct S52ConditionalInstruction
{
  std::string conditionId;
  S52ConditionalOpcode opcode{S52ConditionalOpcode::kUnknown};
};

using S52Instruction = std::variant<
  S52PointSymbolInstruction,
  S52LineStyleInstruction,
  S52AreaPatternInstruction,
  S52TextInstruction,
  S52ConditionalInstruction>;

[[nodiscard]] inline S52ConditionalInstruction makeConditionalInstruction(std::string_view conditionId)
{
  return {std::string(conditionId), parseConditionalOpcode(conditionId)};
}

[[nodiscard]] inline S52InstructionType instructionType(const S52Instruction &instruction) noexcept
{
  return std::visit(
    [](const auto &typedInstruction) noexcept {
      using T = std::decay_t<decltype(typedInstruction)>;
      if constexpr(std::is_same_v<T, S52PointSymbolInstruction>) {
        return S52InstructionType::kPointSymbol;
      } else if constexpr(std::is_same_v<T, S52LineStyleInstruction>) {
        return S52InstructionType::kLineStyle;
      } else if constexpr(std::is_same_v<T, S52AreaPatternInstruction>) {
        return S52InstructionType::kAreaPattern;
      } else if constexpr(std::is_same_v<T, S52TextInstruction>) {
        return S52InstructionType::kTextLabel;
      } else {
        return S52InstructionType::kConditional;
      }
    },
    instruction);
}

[[nodiscard]] inline std::string_view instructionAssetId(const S52Instruction &instruction) noexcept
{
  return std::visit(
    [](const auto &typedInstruction) -> std::string_view {
      using T = std::decay_t<decltype(typedInstruction)>;
      if constexpr(std::is_same_v<T, S52ConditionalInstruction>) {
        return {};
      } else {
        return typedInstruction.assetId;
      }
    },
    instruction);
}

[[nodiscard]] inline std::string_view instructionStyleKey(const S52Instruction &instruction) noexcept
{
  return std::visit(
    [](const auto &typedInstruction) -> std::string_view {
      using T = std::decay_t<decltype(typedInstruction)>;
      if constexpr(std::is_same_v<T, S52ConditionalInstruction>) {
        return {};
      } else {
        return typedInstruction.styleKey;
      }
    },
    instruction);
}

[[nodiscard]] inline std::string_view instructionTextAttributeKey(const S52Instruction &instruction) noexcept
{
  if(const auto *textInstruction = std::get_if<S52TextInstruction>(&instruction)) {
    return textInstruction->attributeKey;
  }

  return {};
}

[[nodiscard]] inline S52ConditionalOpcode instructionConditionalOpcode(const S52Instruction &instruction) noexcept
{
  if(const auto *conditionalInstruction = std::get_if<S52ConditionalInstruction>(&instruction)) {
    return conditionalInstruction->opcode;
  }

  return S52ConditionalOpcode::kUnknown;
}

} // namespace chart_view::runtime::portrayal

#endif
