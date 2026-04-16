#include "s52_presentation_assets.hpp"

#include <algorithm>
#include <cctype>

namespace chart_view::runtime::portrayal {

S52PresentationAssets::S52PresentationAssets()
{
  registerColor("CHBLK", {24U, 38U, 55U, 255U});
  registerColor("CHBRN", {110U, 96U, 52U, 255U});
  registerColor("CHGRD", {70U, 70U, 70U, 255U});
  registerColor("CHGRN", {24U, 116U, 86U, 255U});
  registerColor("CHRED", {160U, 58U, 58U, 255U});
  registerColor("CHYLW", {220U, 176U, 32U, 255U});
  registerColor("DEPDW", {162U, 201U, 229U, 255U});
  registerColor("DEPSC", {44U, 91U, 134U, 255U});
  registerColor("DNGHL", {210U, 92U, 28U, 255U});
  registerColor("LANDF", {196U, 190U, 137U, 255U});
  registerColor("NODTA", {230U, 230U, 217U, 255U});
  registerColor("RESDR", {229U, 196U, 196U, 255U});

  registerPointSymbol("SOUNDG01", "CHBLK", 4);
  registerPointSymbol("BOYSPP01", "CHYLW", 4);
  registerPointSymbol("BOYSPP02", "CHYLW", 3);
  registerPointSymbol("BCNSPP01", "CHBRN", 4);
  registerPointSymbol("BCNSPP02", "CHBRN", 3);
  registerPointSymbol("DANGER01", "DNGHL", 4);
  registerPointSymbol("LNDMRK01", "CHGRD", 4);

  registerLineStyle("DEPCN01", "CHBLK", 2);
  registerLineStyle("COALNE01", "CHBLK", 2);
  registerLineStyle("FAIRWY01", "CHGRN", 2);

  registerAreaPattern("DEPARE01", "DEPDW", "DEPSC", "NODTA", 1, 204U);
  registerAreaPattern("LNDARE01", "LANDF", "CHBRN", "NODTA", 1, 255U);
  registerAreaPattern("RESARE01", "RESDR", "CHRED", "NODTA", 1, 220U);
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
