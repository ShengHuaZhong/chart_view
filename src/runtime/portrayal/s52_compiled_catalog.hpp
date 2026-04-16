#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S52_COMPILED_CATALOG_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S52_COMPILED_CATALOG_HPP

#include "s52_presentation_assets.hpp"
#include "s52_source_catalog.hpp"

#include <string>
#include <vector>

namespace chart_view::runtime::portrayal {

struct S52CompiledLookupInstruction
{
  S52CompiledInstructionType type{S52CompiledInstructionType::kPointSymbol};
  std::string assetId;
  std::string styleKey;
};

struct S52CompiledLookupRow
{
  std::string ruleId;
  std::string objectAcronym;
  chart_data::GeometryType geometryType{chart_data::GeometryType::kPoint};
  std::string displayCategory;
  std::vector<S52CompiledLookupInstruction> instructions;
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
