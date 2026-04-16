#include "s52_presentation_assets.hpp"
#include "s52_source_catalog_compiler.hpp"

#include <algorithm>
#include <cctype>

namespace chart_view::runtime::portrayal {

S52PresentationAssets::S52PresentationAssets()
{
  const auto compiledCatalog = S52SourceCatalogCompiler::compileBuiltin();
  for(const auto &color : compiledCatalog.colors) {
    registerColor(color.token, color.color);
  }
  for(const auto &symbol : compiledCatalog.pointSymbols) {
    registerPointSymbol(symbol.assetId, symbol.colorToken, symbol.radius);
  }
  for(const auto &lineStyle : compiledCatalog.lineStyles) {
    registerLineStyle(lineStyle.assetId, lineStyle.colorToken, lineStyle.thickness);
  }
  for(const auto &areaPattern : compiledCatalog.areaPatterns) {
    registerAreaPattern(
      areaPattern.assetId,
      areaPattern.fillColorToken,
      areaPattern.outlineColorToken,
      areaPattern.holeFillColorToken,
      areaPattern.outlineThickness,
      areaPattern.fillAlpha);
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

void S52PresentationAssets::registerPointSymbol(
  std::string_view assetId,
  std::string_view colorToken,
  int radius)
{
  if(assetId.empty() || colorToken.empty()) {
    return;
  }

  const auto key = normalizeKey(assetId);
  m_pointSymbols[key] = {std::string(assetId), std::string(colorToken), radius};
}

void S52PresentationAssets::registerLineStyle(
  std::string_view assetId,
  std::string_view colorToken,
  int thickness)
{
  if(assetId.empty() || colorToken.empty()) {
    return;
  }

  const auto key = normalizeKey(assetId);
  m_lineStyles[key] = {std::string(assetId), std::string(colorToken), thickness};
}

void S52PresentationAssets::registerAreaPattern(std::string_view assetId,
                                                std::string_view fillColorToken,
                                                std::string_view outlineColorToken,
                                                std::string_view holeFillColorToken,
                                                int outlineThickness,
                                                std::uint8_t fillAlpha)
{
  if(assetId.empty() || fillColorToken.empty() || outlineColorToken.empty()
     || holeFillColorToken.empty()) {
    return;
  }

  const auto key = normalizeKey(assetId);
  m_areaPatterns[key] = {
    std::string(assetId),
    std::string(fillColorToken),
    std::string(outlineColorToken),
    std::string(holeFillColorToken),
    outlineThickness,
    fillAlpha};
}

}// namespace chart_view::runtime::portrayal
