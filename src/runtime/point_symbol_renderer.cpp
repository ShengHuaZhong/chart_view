#include "point_symbol_renderer.hpp"

#include <array>
#include <algorithm>
#include <cctype>
#include <string>

namespace chart_view::runtime {

namespace {

struct PointSymbolGlyph
{
  std::string_view styleKey;
  std::array<std::string_view, 7> rows;
  int anchorX{3};
  int anchorY{3};
};

constexpr PointSymbolGlyph kGlyphs[] = {
  {
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

const PointSymbolGlyph *findGlyph(std::string_view styleKey) noexcept
{
  std::string normalized(styleKey);
  std::transform(
    normalized.begin(),
    normalized.end(),
    normalized.begin(),
    [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  for(const auto &glyph : kGlyphs) {
    if(normalized == glyph.styleKey) {
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

}// namespace

bool PointSymbolRenderer::render(
  std::string_view styleKey,
  SurfacePoint anchor,
  const portrayal::SymbolRule &rule,
  RhiRenderBackend &backend) const
{
  const auto *glyph = findGlyph(styleKey);
  if(glyph == nullptr) {
    return false;
  }

  for(std::size_t y = 0; y < glyph->rows.size(); ++y) {
    const auto row = glyph->rows[y];
    for(std::size_t x = 0; x < row.size(); ++x) {
      if(row[x] != '#') {
        continue;
      }

      drawGlyphPixel(
        backend,
        anchor,
        static_cast<int>(x) - glyph->anchorX,
        static_cast<int>(y) - glyph->anchorY,
        rule.color);
    }
  }

  return true;
}

}// namespace chart_view::runtime
