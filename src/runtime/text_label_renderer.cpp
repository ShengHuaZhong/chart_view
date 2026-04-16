#include "text_label_renderer.hpp"

#include <algorithm>
#include <array>
#include <cctype>
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

const GlyphRows &glyphForChar(char ch) noexcept
{
  switch(std::toupper(static_cast<unsigned char>(ch))) {
  case 'A':
    return kGlyphA;
  case 'B':
    return kGlyphB;
  case 'C':
    return kGlyphC;
  case 'D':
    return kGlyphD;
  case 'E':
    return kGlyphE;
  case 'F':
    return kGlyphF;
  case 'G':
    return kGlyphG;
  case 'H':
    return kGlyphH;
  case 'I':
    return kGlyphI;
  case 'J':
    return kGlyphJ;
  case 'K':
    return kGlyphK;
  case 'L':
    return kGlyphL;
  case 'M':
    return kGlyphM;
  case 'N':
    return kGlyphN;
  case 'O':
    return kGlyphO;
  case 'P':
    return kGlyphP;
  case 'Q':
    return kGlyphQ;
  case 'R':
    return kGlyphR;
  case 'S':
    return kGlyphS;
  case 'T':
    return kGlyphT;
  case 'U':
    return kGlyphU;
  case 'V':
    return kGlyphV;
  case 'W':
    return kGlyphW;
  case 'X':
    return kGlyphX;
  case 'Y':
    return kGlyphY;
  case 'Z':
    return kGlyphZ;
  case '0':
    return kGlyph0;
  case '1':
    return kGlyph1;
  case '2':
    return kGlyph2;
  case '3':
    return kGlyph3;
  case '4':
    return kGlyph4;
  case '5':
    return kGlyph5;
  case '6':
    return kGlyph6;
  case '7':
    return kGlyph7;
  case '8':
    return kGlyph8;
  case '9':
    return kGlyph9;
  case '-':
    return kGlyphDash;
  case '.':
    return kGlyphDot;
  case ' ':
    return kSpaceGlyph;
  default:
    return kUnknownGlyph;
  }
}

std::string extractLabelText(const chart_data::Feature &feature)
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

  if(text.size() > kMaxLabelLength) {
    text.resize(kMaxLabelLength);
  }
  return text;
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
  item.text = std::move(text);
  item.color = rule.color;
  item.pixelSize = rule.pixelSize;

  const auto scale = glyphScale(rule.pixelSize);
  item.width = static_cast<int>(item.text.size()) * static_cast<int>(kGlyphWidth + 1) * scale - scale;
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
  for(char ch : label.text) {
    const auto &glyph = glyphForChar(ch);
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
