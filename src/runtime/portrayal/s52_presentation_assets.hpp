#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S52_PRESENTATION_ASSETS_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S52_PRESENTATION_ASSETS_HPP

#include "s52_source_catalog.hpp"

#include "../render_types.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace chart_view::runtime::portrayal {

struct S52ColorAsset
{
  std::string token;
  SurfaceColor color{0U, 0U, 0U, 255U};
  S52PaletteId palette{S52PaletteId::kDay};
  std::string tableName;
};

struct S52PointSymbolAsset
{
  std::string assetId;
  std::string colorToken;
  int radius{4};
  std::string sourceRcid;
  std::string description;
  S52SourceGraphicMetrics bitmapMetrics;
  S52SourceGraphicMetrics vectorMetrics;
  bool preferBitmap{false};
};

struct S52LineStyleAsset
{
  std::string assetId;
  std::string colorToken;
  int thickness{2};
  std::string sourceRcid;
  std::string description;
  std::string hpgl;
  S52SourceGraphicMetrics vectorMetrics;
};

struct S52AreaPatternAsset
{
  std::string assetId;
  std::string fillColorToken;
  std::string outlineColorToken;
  std::string holeFillColorToken;
  int outlineThickness{1};
  std::uint8_t fillAlpha{255U};
  std::string sourceRcid;
  std::string description;
  std::string fillType;
  std::string spacingToken;
  std::string hpgl;
  std::string primaryColorToken;
  S52SourceGraphicMetrics bitmapMetrics;
  S52SourceGraphicMetrics vectorMetrics;
};

class S52PresentationAssets
{
public:
  S52PresentationAssets();
  explicit S52PresentationAssets(S52PaletteId palette);

  [[nodiscard]] static std::string normalizeKey(std::string_view value);

  [[nodiscard]] const S52ColorAsset *findColor(std::string_view token) const noexcept;
  [[nodiscard]] const S52PointSymbolAsset *findPointSymbol(std::string_view assetId) const noexcept;
  [[nodiscard]] const S52LineStyleAsset *findLineStyle(std::string_view assetId) const noexcept;
  [[nodiscard]] const S52AreaPatternAsset *findAreaPattern(std::string_view assetId) const noexcept;

  [[nodiscard]] SurfaceColor resolveColor(
    std::string_view token,
    SurfaceColor fallback) const noexcept;

private:
  void registerColor(std::string_view token, SurfaceColor color);
  void registerPointSymbol(const S52PointSymbolAsset &asset);
  void registerLineStyle(const S52LineStyleAsset &asset);
  void registerAreaPattern(const S52AreaPatternAsset &asset);

  S52PaletteId m_palette{S52PaletteId::kDay};
  std::unordered_map<std::string, S52ColorAsset> m_colors;
  std::unordered_map<std::string, S52PointSymbolAsset> m_pointSymbols;
  std::unordered_map<std::string, S52LineStyleAsset> m_lineStyles;
  std::unordered_map<std::string, S52AreaPatternAsset> m_areaPatterns;
};

}// namespace chart_view::runtime::portrayal

#endif
