#include "text_label_renderer.hpp"

#include <algorithm>
#include <array>
#include <string_view>

namespace chart_view::runtime {

namespace {

constexpr std::size_t kGlyphHeight = 5;
constexpr std::size_t kGlyphWidth = 3;
constexpr std::size_t kMaxLabelLength = 24;

using GlyphRows = std::array<std::string_view, kGlyphHeight>;

constexpr GlyphRows kSpaceGlyph{
  "...",
  "...",
  "...",
  "...",
  "...",
};

constexpr GlyphRows kUnknownGlyph{
  "###",
  "..#",
  ".#.",
  "...",
  ".#.",
};

constexpr GlyphRows kGlyphA{".#.","#.#","###","#.#","#.#"};
constexpr GlyphRows kGlyphB{"##.","#.#","##.","#.#","##."};
constexpr GlyphRows kGlyphC{".##","#..","#..","#..",".##"};
constexpr GlyphRows kGlyphD{"##.","#.#","#.#","#.#","##."};
constexpr GlyphRows kGlyphE{"###","#..","##.","#..","###"};
constexpr GlyphRows kGlyphF{"###","#..","##.","#..","#.."};
constexpr GlyphRows kGlyphG{".##","#..","#.#","#.#",".##"};
constexpr GlyphRows kGlyphH{"#.#","#.#","###","#.#","#.#"};
constexpr GlyphRows kGlyphI{"###",".#.",".#.",".#.","###"};
constexpr GlyphRows kGlyphJ{"..#","..#","..#","#.#",".#."};
constexpr GlyphRows kGlyphK{"#.#","#.#","##.","#.#","#.#"};
constexpr GlyphRows kGlyphL{"#..","#..","#..","#..","###"};
constexpr GlyphRows kGlyphM{"#.#","###","###","#.#","#.#"};
constexpr GlyphRows kGlyphN{"#.#","###","###","###","#.#"};
constexpr GlyphRows kGlyphO{".#.","#.#","#.#","#.#",".#."};
constexpr GlyphRows kGlyphP{"##.","#.#","##.","#..","#.."};
constexpr GlyphRows kGlyphQ{".#.","#.#","#.#",".#.","..#"};
constexpr GlyphRows kGlyphR{"##.","#.#","##.","#.#","#.#"};
constexpr GlyphRows kGlyphS{".##","#..",".#.","..#","##."};
constexpr GlyphRows kGlyphT{"###",".#.",".#.",".#.",".#."};
constexpr GlyphRows kGlyphU{"#.#","#.#","#.#","#.#","###"};
constexpr GlyphRows kGlyphV{"#.#","#.#","#.#","#.#",".#."};
constexpr GlyphRows kGlyphW{"#.#","#.#","###","###","#.#"};
constexpr GlyphRows kGlyphX{"#.#","#.#",".#.","#.#","#.#"};
constexpr GlyphRows kGlyphY{"#.#","#.#",".#.",".#.",".#."};
constexpr GlyphRows kGlyphZ{"###","..#",".#.","#..","###"};
constexpr GlyphRows kGlyph0{".#.","#.#","#.#","#.#",".#."};
constexpr GlyphRows kGlyph1{".#.","##.",".#.",".#.","###"};
constexpr GlyphRows kGlyph2{"###","..#","###","#..","###"};
constexpr GlyphRows kGlyph3{"###","..#",".##","..#","###"};
constexpr GlyphRows kGlyph4{"#.#","#.#","###","..#","..#"};
constexpr GlyphRows kGlyph5{"###","#..","###","..#","###"};
constexpr GlyphRows kGlyph6{".##","#..","###","#.#","###"};
constexpr GlyphRows kGlyph7{"###","..#",".#.",".#.",".#."};
constexpr GlyphRows kGlyph8{"###","#.#","###","#.#","###"};
constexpr GlyphRows kGlyph9{"###","#.#","###","..#","##."};
constexpr GlyphRows kGlyphDash{"...","...","###","...","..."};
constexpr GlyphRows kGlyphDot{"...","...","...","...",".#."};

char32_t toUpperAscii(char32_t codePoint) noexcept
{
  if(codePoint >= U'a' && codePoint <= U'z') {
    return codePoint - (U'a' - U'A');
  }

  return codePoint;
}

const GlyphRows &glyphForCodePoint(char32_t codePoint) noexcept
{
  switch(toUpperAscii(codePoint)) {
  case U'A':
    return kGlyphA;
  case U'B':
    return kGlyphB;
  case U'C':
    return kGlyphC;
  case U'D':
    return kGlyphD;
  case U'E':
    return kGlyphE;
  case U'F':
    return kGlyphF;
  case U'G':
    return kGlyphG;
  case U'H':
    return kGlyphH;
  case U'I':
    return kGlyphI;
  case U'J':
    return kGlyphJ;
  case U'K':
    return kGlyphK;
  case U'L':
    return kGlyphL;
  case U'M':
    return kGlyphM;
  case U'N':
    return kGlyphN;
  case U'O':
    return kGlyphO;
  case U'P':
    return kGlyphP;
  case U'Q':
    return kGlyphQ;
  case U'R':
    return kGlyphR;
  case U'S':
    return kGlyphS;
  case U'T':
    return kGlyphT;
  case U'U':
    return kGlyphU;
  case U'V':
    return kGlyphV;
  case U'W':
    return kGlyphW;
  case U'X':
    return kGlyphX;
  case U'Y':
    return kGlyphY;
  case U'Z':
    return kGlyphZ;
  case U'0':
    return kGlyph0;
  case U'1':
    return kGlyph1;
  case U'2':
    return kGlyph2;
  case U'3':
    return kGlyph3;
  case U'4':
    return kGlyph4;
  case U'5':
    return kGlyph5;
  case U'6':
    return kGlyph6;
  case U'7':
    return kGlyph7;
  case U'8':
    return kGlyph8;
  case U'9':
    return kGlyph9;
  case U'-':
    return kGlyphDash;
  case U'.':
    return kGlyphDot;
  case U' ':
    return kSpaceGlyph;
  default:
    return kUnknownGlyph;
  }
}

text::UnicodeText extractLabelText(const chart_data::Feature &feature)
{
  const auto extractString = [&](std::string_view key) -> std::string {
    const auto it = feature.attributes.find(std::string(key));
    if(it == feature.attributes.end()) {
      return {};
    }

    const auto *value = std::get_if<std::string>(&it->second);
    return value == nullptr ? std::string{} : *value;
  };

  auto text = extractString("OBJNAM");
  if(text.empty()) {
    text = extractString("NOBJNM");
  }

  return runtime::text::decodeUtf8(text, kMaxLabelLength);
}

int glyphScale(std::uint32_t pixelSize) noexcept
{
  return std::max(1, static_cast<int>(pixelSize / 6U));
}

}// namespace

std::optional<LabelItem> TextLabelRenderer::layout(
  std::string_view textKey,
  const chart_data::Feature &feature,
  SurfacePoint anchor,
  const portrayal::TextRule &rule) const
{
  if(textKey.empty()) {
    return std::nullopt;
  }

  auto text = extractLabelText(feature);
  if(text.empty()) {
    return std::nullopt;
  }

  LabelItem item;
  item.text = std::move(text.utf8);
  item.glyphText = std::move(text.codePoints);
  item.color = rule.color;
  item.pixelSize = rule.pixelSize;

  const auto scale = glyphScale(rule.pixelSize);
  item.width = static_cast<int>(item.glyphText.size()) * static_cast<int>(kGlyphWidth + 1) * scale - scale;
  item.height = static_cast<int>(kGlyphHeight) * scale;
  item.origin = {
    anchor.x + 4,
    anchor.y - item.height / 2};

  return item;
}

void TextLabelRenderer::render(const LabelItem &label, RhiRenderBackend &backend) const
{
  const auto scale = glyphScale(label.pixelSize);
  int cursorX = label.origin.x;
  for(char32_t codePoint : label.glyphText) {
    const auto &glyph = glyphForCodePoint(codePoint);
    for(std::size_t y = 0; y < glyph.size(); ++y) {
      for(std::size_t x = 0; x < glyph[y].size(); ++x) {
        if(glyph[y][x] != '#') {
          continue;
        }

        for(int dy = 0; dy < scale; ++dy) {
          for(int dx = 0; dx < scale; ++dx) {
            backend.drawPoint(
              {cursorX + static_cast<int>(x) * scale + dx, label.origin.y + static_cast<int>(y) * scale + dy},
              0,
              label.color);
          }
        }
      }
    }

    cursorX += static_cast<int>(kGlyphWidth + 1) * scale;
  }
}

}// namespace chart_view::runtime
