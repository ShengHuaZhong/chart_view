#include "point_symbol_renderer.hpp"
#include "portrayal/s52_presentation_assets.hpp"

#include <array>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

namespace chart_view::runtime {

namespace {

constexpr int kGlyphExtent = 7;

struct PointSymbolGlyph
{
  std::string_view assetId;
  std::string_view styleKey;
  std::array<std::string_view, kGlyphExtent> rows;
  int anchorX{3};
  int anchorY{3};
};

constexpr PointSymbolGlyph kGlyphs[] = {
  {
    "SOUNDG01",
    "point/sounding",
    {
      "...#...",
      "...#...",
      "...#...",
      "#######",
      "...#...",
      "...#...",
      "...#...",
    },
  },
  {
    "BOYSPP01",
    "point/buoy",
    {
      "...#...",
      "..###..",
      ".#.#.#.",
      "...#...",
      "..###..",
      "..###..",
      "...#...",
    },
  },
  {
    "BOYSPP02",
    "point/buoy",
    {
      ".......",
      "...#...",
      "..###..",
      ".#####.",
      "..###..",
      "..###..",
      ".......",
    },
  },
  {
    "BCNSPP01",
    "point/beacon",
    {
      "...#...",
      "..###..",
      "...#...",
      "..###..",
      "..###..",
      "..###..",
      ".#####.",
    },
  },
  {
    "BCNSPP02",
    "point/beacon",
    {
      ".......",
      "...#...",
      "...#...",
      "..###..",
      "..###..",
      ".#####.",
      ".#####.",
    },
  },
  {
    "DANGER01",
    "point/danger",
    {
      "#.....#",
      ".#...#.",
      "..#.#..",
      "...#...",
      "..#.#..",
      ".#...#.",
      "#.....#",
    },
  },
  {
    "LNDMRK01",
    "point/landmark",
    {
      "...#...",
      "..###..",
      "...#...",
      "..###..",
      "..###..",
      ".#####.",
      ".#####.",
    },
  },
};

std::string normalizeKey(std::string_view value)
{
  std::string normalized(value);
  std::transform(
    normalized.begin(),
    normalized.end(),
    normalized.begin(),
    [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return normalized;
}

const PointSymbolGlyph *findGlyph(std::string_view assetId, std::string_view styleKey) noexcept
{
  const auto normalizedAssetId = normalizeKey(assetId);
  if(!normalizedAssetId.empty()) {
    for(const auto &glyph : kGlyphs) {
      if(normalizedAssetId == normalizeKey(glyph.assetId)) {
        return &glyph;
      }
    }
  }

  const auto normalizedStyleKey = normalizeKey(styleKey);
  for(const auto &glyph : kGlyphs) {
    if(normalizedStyleKey == glyph.styleKey) {
      return &glyph;
    }
  }

  return nullptr;
}

void drawGlyphPixel(
  RhiRenderBackend &backend,
  SurfacePoint anchor,
  int offsetX,
  int offsetY,
  SurfaceColor color) noexcept
{
  backend.drawPoint({anchor.x + offsetX, anchor.y + offsetY}, 0, color);
}

const portrayal::S52PresentationAssets &presentationAssets()
{
  static const auto *assets = new portrayal::S52PresentationAssets();
  return *assets;
}

const portrayal::S52PointSymbolAsset *findCompiledAsset(std::string_view assetId) noexcept
{
  return assetId.empty() ? nullptr : presentationAssets().findPointSymbol(assetId);
}

const portrayal::S52SourceGraphicMetrics *preferredMetrics(
  const portrayal::S52PointSymbolAsset &asset) noexcept
{
  const auto bitmapAvailable = asset.bitmapMetrics.width > 0 || asset.bitmapMetrics.height > 0;
  const auto vectorAvailable = asset.vectorMetrics.width > 0 || asset.vectorMetrics.height > 0;
  if(asset.preferBitmap && bitmapAvailable) {
    return &asset.bitmapMetrics;
  }

  if(vectorAvailable) {
    return &asset.vectorMetrics;
  }

  if(bitmapAvailable) {
    return &asset.bitmapMetrics;
  }

  return nullptr;
}

int glyphAnchorFromMetrics(
  int baseAnchor,
  const portrayal::S52SourceGraphicMetrics *metrics,
  bool useX) noexcept
{
  if(metrics == nullptr) {
    return baseAnchor;
  }

  const auto &pivot = metrics->pivot;
  const auto &origin = metrics->origin;
  const auto metricExtent = useX ? metrics->width : metrics->height;
  if(metricExtent <= 0 || !pivot.valid || !origin.valid) {
    return baseAnchor;
  }

  const auto delta = static_cast<double>((useX ? origin.x : origin.y) - (useX ? pivot.x : pivot.y));
  const auto offset =
    static_cast<int>(std::lround(delta / static_cast<double>(metricExtent) * static_cast<double>(kGlyphExtent)));
  return std::clamp(baseAnchor + offset, 0, kGlyphExtent - 1);
}

SurfacePoint offsetAnchorFromMetrics(
  SurfacePoint anchor,
  const portrayal::S52SourceGraphicMetrics *metrics) noexcept
{
  if(metrics == nullptr) {
    return anchor;
  }

  if(metrics->width <= 0 || metrics->height <= 0 || !metrics->pivot.valid || !metrics->origin.valid) {
    return anchor;
  }

  const auto offsetX = static_cast<int>(std::lround(
    static_cast<double>(metrics->origin.x - metrics->pivot.x)
    / static_cast<double>(metrics->width) * static_cast<double>(kGlyphExtent)));
  const auto offsetY = static_cast<int>(std::lround(
    static_cast<double>(metrics->origin.y - metrics->pivot.y)
    / static_cast<double>(metrics->height) * static_cast<double>(kGlyphExtent)));
  return {anchor.x + offsetX, anchor.y + offsetY};
}

void drawGenericAssetSymbol(
  const portrayal::S52PointSymbolAsset &asset,
  SurfacePoint anchor,
  SurfaceColor color,
  RhiRenderBackend &backend) noexcept
{
  const auto *metrics = preferredMetrics(asset);
  const auto adjustedAnchor = offsetAnchorFromMetrics(anchor, metrics);
  backend.drawPoint(adjustedAnchor, std::max(0, asset.radius / 4), color);

  if(asset.preferBitmap) {
    backend.drawPoint({adjustedAnchor.x - 1, adjustedAnchor.y - 1}, 0, color);
    backend.drawPoint({adjustedAnchor.x + 1, adjustedAnchor.y - 1}, 0, color);
    backend.drawPoint({adjustedAnchor.x - 1, adjustedAnchor.y + 1}, 0, color);
    backend.drawPoint({adjustedAnchor.x + 1, adjustedAnchor.y + 1}, 0, color);
    return;
  }

  backend.drawPoint({adjustedAnchor.x - 1, adjustedAnchor.y}, 0, color);
  backend.drawPoint({adjustedAnchor.x + 1, adjustedAnchor.y}, 0, color);
  backend.drawPoint({adjustedAnchor.x, adjustedAnchor.y - 1}, 0, color);
  backend.drawPoint({adjustedAnchor.x, adjustedAnchor.y + 1}, 0, color);
}

}// namespace

bool PointSymbolRenderer::render(
  std::string_view assetId,
  std::string_view styleKey,
  SurfacePoint anchor,
  const portrayal::SymbolRule &rule,
  RhiRenderBackend &backend) const
{
  const auto *glyph = findGlyph(assetId, styleKey);
  const auto *asset = findCompiledAsset(assetId);
  if(glyph == nullptr) {
    if(asset == nullptr) {
      return false;
    }

    drawGenericAssetSymbol(*asset, anchor, rule.color, backend);
    return true;
  }

  const auto *metrics = asset == nullptr ? nullptr : preferredMetrics(*asset);
  const auto anchorX = glyphAnchorFromMetrics(glyph->anchorX, metrics, true);
  const auto anchorY = glyphAnchorFromMetrics(glyph->anchorY, metrics, false);

  for(std::size_t y = 0; y < glyph->rows.size(); ++y) {
    const auto row = glyph->rows[y];
    for(std::size_t x = 0; x < row.size(); ++x) {
      if(row[x] != '#') {
        continue;
      }

      drawGlyphPixel(
        backend,
        anchor,
        static_cast<int>(x) - anchorX,
        static_cast<int>(y) - anchorY,
        rule.color);
    }
  }

  return true;
}

}// namespace chart_view::runtime
