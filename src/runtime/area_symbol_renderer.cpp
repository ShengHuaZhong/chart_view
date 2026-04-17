#include "area_symbol_renderer.hpp"
#include "portrayal/s52_presentation_assets.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
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

const portrayal::S52PresentationAssets &presentationAssets()
{
  static const auto *assets = new portrayal::S52PresentationAssets();
  return *assets;
}

int resolvePatternSpacing(const portrayal::S52AreaPatternAsset *asset) noexcept
{
  if(asset == nullptr) {
    return 6;
  }

  const auto dominantMetric = std::max(
    std::max(asset->bitmapMetrics.width, asset->bitmapMetrics.height),
    std::max(asset->vectorMetrics.width, asset->vectorMetrics.height));
  const auto normalizedSpacing = normalizeStyleKey(asset->spacingToken);
  if(!normalizedSpacing.empty() && std::all_of(
      normalizedSpacing.begin(),
      normalizedSpacing.end(),
      [](unsigned char ch) { return std::isdigit(ch) != 0; })) {
    return std::clamp(std::stoi(normalizedSpacing), 4, 16);
  }

  if(normalizedSpacing == "c") {
    return std::clamp(dominantMetric > 0 ? dominantMetric / 32 : 6, 4, 16);
  }

  if(normalizedSpacing == "s") {
    return std::clamp(dominantMetric > 0 ? dominantMetric / 24 : 8, 5, 18);
  }

  if(normalizedSpacing == "l") {
    return std::clamp(dominantMetric > 0 ? dominantMetric / 16 : 10, 6, 20);
  }

  return 6;
}

void drawBoundaryMarkers(std::span<const SurfacePoint> ring,
                         int spacing,
                         SurfaceColor color,
                         RhiRenderBackend &backend)
{
  if(ring.size() < 2 || spacing <= 0) {
    return;
  }

  for(std::size_t i = 0; i < ring.size(); ++i) {
    const auto start = ring[i];
    const auto end = ring[(i + 1) % ring.size()];
    const auto dx = static_cast<double>(end.x - start.x);
    const auto dy = static_cast<double>(end.y - start.y);
    const auto length = std::sqrt(dx * dx + dy * dy);
    if(length <= 1e-6) {
      continue;
    }

    for(double distance = spacing / 2.0; distance < length; distance += static_cast<double>(spacing)) {
      const auto t = distance / length;
      const auto x = static_cast<int>(std::lround(
        static_cast<double>(start.x) + dx * t));
      const auto y = static_cast<int>(std::lround(
        static_cast<double>(start.y) + dy * t));
      backend.drawPoint({x, y}, 0, color);
    }
  }
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
  const auto *asset = presentationAssets().findAreaPattern(assetId);
  const auto knownAreaStyle =
    normalizedStyleKey.rfind("area/", 0) == 0 && normalizedStyleKey != "area/default";
  if((asset == nullptr && !knownAreaStyle && normalizedAssetId != "depare01") || exterior.size() < 3) {
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

  const auto patternSpacing = resolvePatternSpacing(asset);
  const int startX = minX + patternSpacing / 2;
  const int startY = minY + patternSpacing / 2;
  for(int y = startY; y <= maxY; y += patternSpacing) {
    for(int x = startX; x <= maxX; x += patternSpacing) {
      const SurfacePoint sample{x, y};
      if(!pointInPolygon(exterior, sample) || pointInsideAnyHole(holes, sample)) {
        continue;
      }

      backend.drawPoint(sample, 0, rule.outlineColor);
    }
  }

  backend.drawClosedPolyline(exterior, rule.outlineThickness, rule.outlineColor);
  const auto symbolizedBoundary =
    normalizedStyleKey.find("symbolized_boundary") != std::string::npos
    || (asset != nullptr && !asset->hpgl.empty()
        && normalizedStyleKey.find("plain_boundary") == std::string::npos);
  if(symbolizedBoundary) {
    drawBoundaryMarkers(exterior, patternSpacing, rule.outlineColor, backend);
  }
  for(const auto &hole : holes) {
    if(hole.size() < 2) {
      continue;
    }
    backend.drawClosedPolyline(hole, rule.outlineThickness, rule.outlineColor);
    if(symbolizedBoundary) {
      drawBoundaryMarkers(hole, patternSpacing, rule.outlineColor, backend);
    }
  }

  return true;
}

}// namespace chart_view::runtime
