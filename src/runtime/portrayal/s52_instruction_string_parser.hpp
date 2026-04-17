#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S52_INSTRUCTION_STRING_PARSER_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S52_INSTRUCTION_STRING_PARSER_HPP

#include "s52_source_catalog.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace chart_view::runtime::portrayal {

struct S52InstructionStringParseResult
{
  std::vector<S52SourceLookupInstruction> instructions;
  std::vector<std::string> unsupportedStatements;
};

class S52InstructionStringParser
{
public:
  [[nodiscard]] static S52InstructionStringParseResult parse(std::string_view rawInstruction);
};

} // namespace chart_view::runtime::portrayal

#endif
