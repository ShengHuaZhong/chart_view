#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S52_SOURCE_CATALOG_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S52_SOURCE_CATALOG_HPP

#include "s52_instruction_ir.hpp"

#include "../chart_data/geometry.hpp"
#include "../render_types.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace chart_view::runtime::portrayal {

enum class S52PaletteId : std::uint8_t
{
  kDay = 0,
  kDusk = 1,
  kNight = 2,
};

struct S52SourceColor
{
  S52PaletteId palette{S52PaletteId::kDay};
  std::string token;
  SurfaceColor color{0U, 0U, 0U, 255U};
  std::string tableName;
  std::string graphicsFile;
};

struct S52SourceGraphicMetrics
{
  struct Anchor
  {
    int x{0};
    int y{0};
    bool valid{false};
  };

  int width{0};
  int height{0};
  Anchor pivot{};
  Anchor origin{};
};

struct S52SourcePointSymbol
{
  std::string assetId;
  std::string colorToken;
  int radius{4};
  std::string sourceRcid;
  std::string description;
  std::string definition;
  S52SourceGraphicMetrics bitmapMetrics;
  S52SourceGraphicMetrics vectorMetrics;
  bool preferBitmap{false};
};

struct S52SourceLineStyle
{
  std::string assetId;
  std::string colorToken;
  int thickness{2};
  std::string sourceRcid;
  std::string description;
  std::string hpgl;
  S52SourceGraphicMetrics vectorMetrics;
};

struct S52SourceAreaPattern
{
  std::string assetId;
  std::string fillColorToken;
  std::string outlineColorToken;
  std::string holeFillColorToken;
  int outlineThickness{1};
  std::uint8_t fillAlpha{255U};
  std::string sourceRcid;
  std::string description;
  std::string definition;
  std::string fillType;
  std::string spacing;
  std::string hpgl;
  std::string primaryColorToken;
  S52SourceGraphicMetrics bitmapMetrics;
  S52SourceGraphicMetrics vectorMetrics;
};

struct S52SourceLookupInstruction
{
  S52InstructionType type{S52InstructionType::kPointSymbol};
  std::string assetId;
  std::string styleKey;
  std::string attributeKey;
};

struct S52SourceLookupRow
{
  std::string objectAcronym;
  chart_data::GeometryType geometryType{chart_data::GeometryType::kPoint};
  std::vector<S52SourceLookupInstruction> instructions;
  std::string displayCategory{"standard"};
  int displayPriority{0};
  std::uint32_t viewGroup{0};
  std::string ruleId;
  std::string sourceLookupId;
  std::string sourceRcid;
  std::string geometryTypeText;
  std::string displayPriorityText;
  std::string radarPriorityText;
  std::string tableName;
  std::vector<std::string> attributeCodes;
  std::string rawInstruction;
  std::string comment;
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
