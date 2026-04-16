#include "area_symbol_renderer.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace chart_view::runtime {

namespace {

std::string normalizeStyleKey(std::string_view value)
{
  std::string normalized(value);
  std::transform(
    normalized.begin(),
    normalized.end(),
    normalized.begin(),
    [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return normalized;
}

bool pointInPolygon(std::span<const SurfacePoint> polygon, SurfacePoint point) noexcept
{
  if(polygon.size() < 3) {
    return false;
  }

  bool inside = false;
  for(std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
    const auto &a = polygon[i];
    const auto &b = polygon[j];
    const bool intersects = ((a.y > point.y) != (b.y > point.y))
        && (static_cast<double>(point.x)
            < static_cast<double>(b.x - a.x) * static_cast<double>(point.y - a.y)
                    / static_cast<double>((b.y - a.y) == 0 ? 1 : (b.y - a.y))
                + static_cast<double>(a.x));
    if(intersects) {
      inside = !inside;
    }
  }

  return inside;
}

bool pointInsideAnyHole(const std::vector<std::vector<SurfacePoint>> &holes, SurfacePoint point) noexcept
{
  return std::any_of(
    holes.begin(),
    holes.end(),
    [&](const auto &hole) { return pointInPolygon(hole, point); });
}

}// namespace

bool AreaSymbolRenderer::render(
  std::string_view assetId,
  std::string_view styleKey,
  std::span<const SurfacePoint> exterior,
  const std::vector<std::vector<SurfacePoint>> &holes,
  const portrayal::AreaFillRule &rule,
  RhiRenderBackend &backend) const
{
  const auto normalizedAssetId = normalizeStyleKey(assetId);
  const auto normalizedStyleKey = normalizeStyleKey(styleKey);
  if((normalizedAssetId != "depare01" && normalizedStyleKey != "area/depth") || exterior.size() < 3) {
    return false;
  }

  backend.fillPolygon(exterior, rule.fillColor);

  for(const auto &hole : holes) {
    if(hole.size() < 3) {
      continue;
    }
    backend.fillPolygon(hole, rule.holeFillColor);
  }

  int minX = exterior.front().x;
  int maxX = exterior.front().x;
  int minY = exterior.front().y;
  int maxY = exterior.front().y;
  for(const auto &point : exterior) {
    minX = std::min(minX, point.x);
    maxX = std::max(maxX, point.x);
    minY = std::min(minY, point.y);
    maxY = std::max(maxY, point.y);
  }

  constexpr int kPatternSpacing = 6;
  const int startX = minX + kPatternSpacing / 2;
  const int startY = minY + kPatternSpacing / 2;
  for(int y = startY; y <= maxY; y += kPatternSpacing) {
    for(int x = startX; x <= maxX; x += kPatternSpacing) {
      const SurfacePoint sample{x, y};
      if(!pointInPolygon(exterior, sample) || pointInsideAnyHole(holes, sample)) {
        continue;
      }

      backend.drawPoint(sample, 0, rule.outlineColor);
    }
  }

  backend.drawClosedPolyline(exterior, rule.outlineThickness, rule.outlineColor);
  for(const auto &hole : holes) {
    if(hole.size() < 2) {
      continue;
    }
    backend.drawClosedPolyline(hole, rule.outlineThickness, rule.outlineColor);
  }

  return true;
}

}// namespace chart_view::runtime
