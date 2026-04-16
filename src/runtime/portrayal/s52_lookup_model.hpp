#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S52_LOOKUP_MODEL_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S52_LOOKUP_MODEL_HPP

#include "s52_instruction_ir.hpp"

#include "../chart_data/feature.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace chart_view::runtime::portrayal {

struct S52LookupResult
{
  std::string lookupKey;
  std::string ruleId;
  std::string displayCategory;
  int displayPriority{0};
  std::uint32_t viewGroup{0};
  std::vector<S52Instruction> instructions;
  bool suppressed{false};
};

class S52LookupModel
{
public:
  [[nodiscard]] static std::optional<S52LookupResult> lookup(const chart_data::Feature &feature);

private:
  [[nodiscard]] static std::string normalizeAcronym(std::string_view value);
};

}// namespace chart_view::runtime::portrayal

#endif
