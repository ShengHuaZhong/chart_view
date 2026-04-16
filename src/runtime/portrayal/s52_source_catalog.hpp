#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S52_SOURCE_CATALOG_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S52_SOURCE_CATALOG_HPP

#include "../chart_data/geometry.hpp"
#include "../render_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace chart_view::runtime::portrayal {

enum class S52PaletteId : std::uint8_t
{
  kDay = 0,
  kDusk = 1,
  kNight = 2,
};

enum class S52CompiledInstructionType : std::uint8_t
{
  kPointSymbol = 0,
  kLineStyle = 1,
  kAreaPattern = 2,
  kTextLabel = 3,
};

struct S52SourceColor
{
  S52PaletteId palette{S52PaletteId::kDay};
  std::string token;
  SurfaceColor color{0U, 0U, 0U, 255U};
};

struct S52SourcePointSymbol
{
  std::string assetId;
  std::string colorToken;
  int radius{4};
};

struct S52SourceLineStyle
{
  std::string assetId;
  std::string colorToken;
  int thickness{2};
};

struct S52SourceAreaPattern
{
  std::string assetId;
  std::string fillColorToken;
  std::string outlineColorToken;
  std::string holeFillColorToken;
  int outlineThickness{1};
  std::uint8_t fillAlpha{255U};
};

struct S52SourceLookupInstruction
{
  S52CompiledInstructionType type{S52CompiledInstructionType::kPointSymbol};
  std::string assetId;
  std::string styleKey;
};

struct S52SourceLookupRow
{
  std::string objectAcronym;
  chart_data::GeometryType geometryType{chart_data::GeometryType::kPoint};
  std::vector<S52SourceLookupInstruction> instructions;
  std::string displayCategory{"standard"};
  std::string ruleId;
};

struct S52SourceCatalog
{
  std::vector<S52SourceColor> colors;
  std::vector<S52SourcePointSymbol> pointSymbols;
  std::vector<S52SourceLineStyle> lineStyles;
  std::vector<S52SourceAreaPattern> areaPatterns;
  std::vector<S52SourceLookupRow> lookupRows;
};

[[nodiscard]] S52SourceCatalog buildBuiltinS52SourceCatalog();

} // namespace chart_view::runtime::portrayal

#endif
