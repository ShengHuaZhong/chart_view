#include "s52_presentation_assets.hpp"
#include "s52_source_catalog_compiler.hpp"

#include <algorithm>
#include <cctype>

namespace chart_view::runtime::portrayal {

namespace {

std::string_view preferredTableName(S52PaletteId palette) noexcept
{
  switch(palette) {
  case S52PaletteId::kDusk:
    return "DUSK";
  case S52PaletteId::kNight:
    return "NIGHT";
  case S52PaletteId::kDay:
  default:
    return "DAY_BRIGHT";
  }
}

void registerPaletteColors(
  std::unordered_map<std::string, S52ColorAsset> &colors,
  const S52CompiledCatalog &compiledCatalog,
  S52PaletteId palette,
  bool strictTableFilter)
{
  const auto wantedTable = S52PresentationAssets::normalizeKey(preferredTableName(palette));
  for(const auto &color : compiledCatalog.colors) {
    if(color.palette != palette) {
      continue;
    }

    const auto colorTable = S52PresentationAssets::normalizeKey(color.tableName);
    if(strictTableFilter && !wantedTable.empty() && !colorTable.empty() && colorTable != wantedTable) {
      continue;
    }

    colors[S52PresentationAssets::normalizeKey(color.token)] = color;
  }
}

}// namespace

S52PresentationAssets::S52PresentationAssets()
  : S52PresentationAssets(S52PaletteId::kDay)
{
}

S52PresentationAssets::S52PresentationAssets(S52PaletteId palette)
  : m_palette(palette)
{
  const auto compiledCatalog = S52SourceCatalogCompiler::compilePreferred();
  registerPaletteColors(m_colors, compiledCatalog, m_palette, true);
  if(m_colors.empty()) {
    registerPaletteColors(m_colors, compiledCatalog, m_palette, false);
  }
  if(m_colors.empty() && m_palette != S52PaletteId::kDay) {
    registerPaletteColors(m_colors, compiledCatalog, S52PaletteId::kDay, true);
    if(m_colors.empty()) {
      registerPaletteColors(m_colors, compiledCatalog, S52PaletteId::kDay, false);
    }
  }
  for(const auto &symbol : compiledCatalog.pointSymbols) {
    registerPointSymbol(symbol);
  }
  for(const auto &lineStyle : compiledCatalog.lineStyles) {
    registerLineStyle(lineStyle);
  }
  for(const auto &areaPattern : compiledCatalog.areaPatterns) {
    registerAreaPattern(areaPattern);
  }
}

std::string S52PresentationAssets::normalizeKey(std::string_view value)
{
  std::string normalized(value);
  std::transform(
    normalized.begin(),
    normalized.end(),
    normalized.begin(),
    [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return normalized;
}

const S52ColorAsset *S52PresentationAssets::findColor(std::string_view token) const noexcept
{
  const auto it = m_colors.find(normalizeKey(token));
  return it == m_colors.end() ? nullptr : &it->second;
}

const S52PointSymbolAsset *S52PresentationAssets::findPointSymbol(
  std::string_view assetId) const noexcept
{
  const auto it = m_pointSymbols.find(normalizeKey(assetId));
  return it == m_pointSymbols.end() ? nullptr : &it->second;
}

const S52LineStyleAsset *S52PresentationAssets::findLineStyle(std::string_view assetId) const noexcept
{
  const auto it = m_lineStyles.find(normalizeKey(assetId));
  return it == m_lineStyles.end() ? nullptr : &it->second;
}

const S52AreaPatternAsset *S52PresentationAssets::findAreaPattern(
  std::string_view assetId) const noexcept
{
  const auto it = m_areaPatterns.find(normalizeKey(assetId));
  return it == m_areaPatterns.end() ? nullptr : &it->second;
}

SurfaceColor S52PresentationAssets::resolveColor(
  std::string_view token,
  SurfaceColor fallback) const noexcept
{
  if(const auto *asset = findColor(token); asset != nullptr) {
    return asset->color;
  }

  return fallback;
}

void S52PresentationAssets::registerColor(std::string_view token, SurfaceColor color)
{
  if(token.empty()) {
    return;
  }

  const auto key = normalizeKey(token);
  m_colors[key] = {std::string(token), color};
}

void S52PresentationAssets::registerPointSymbol(const S52PointSymbolAsset &asset)
{
  if(asset.assetId.empty()) {
    return;
  }

  const auto key = normalizeKey(asset.assetId);
  m_pointSymbols[key] = asset;
}

void S52PresentationAssets::registerLineStyle(const S52LineStyleAsset &asset)
{
  if(asset.assetId.empty() || asset.colorToken.empty()) {
    return;
  }

  const auto key = normalizeKey(asset.assetId);
  m_lineStyles[key] = asset;
}

void S52PresentationAssets::registerAreaPattern(const S52AreaPatternAsset &asset)
{
  if(asset.assetId.empty() || asset.fillColorToken.empty() || asset.outlineColorToken.empty()
     || asset.holeFillColorToken.empty()) {
    return;
  }

  const auto key = normalizeKey(asset.assetId);
  m_areaPatterns[key] = asset;
}

}// namespace chart_view::runtime::portrayal
