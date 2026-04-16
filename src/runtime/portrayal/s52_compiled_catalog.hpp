#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S52_COMPILED_CATALOG_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S52_COMPILED_CATALOG_HPP

#include "s52_instruction_ir.hpp"
#include "s52_presentation_assets.hpp"
#include "s52_source_catalog.hpp"

#include <string>
#include <vector>

namespace chart_view::runtime::portrayal {

struct S52CompiledLookupRow
{
  std::string ruleId;
  std::string objectAcronym;
  chart_data::GeometryType geometryType{chart_data::GeometryType::kPoint};
  std::string displayCategory;
  int displayPriority{0};
  std::uint32_t viewGroup{0};
  std::vector<S52Instruction> instructions;
};

struct S52CompiledCatalog
{
  std::vector<S52ColorAsset> colors;
  std::vector<S52PointSymbolAsset> pointSymbols;
  std::vector<S52LineStyleAsset> lineStyles;
  std::vector<S52AreaPatternAsset> areaPatterns;
  std::vector<S52CompiledLookupRow> lookupRows;
};

} // namespace chart_view::runtime::portrayal

#endif
