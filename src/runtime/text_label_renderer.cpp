#include "text_label_renderer.hpp"

#include <algorithm>
#include <string_view>

namespace chart_view::runtime {

namespace {

constexpr std::size_t kMaxLabelLength = 24;

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
  item.width = 0;
  item.height = 0;
  item.baselineOffset = 0;

  for(const auto codePoint : item.glyphText) {
    const auto &glyph = m_glyphCache.glyphFor(codePoint, rule.pixelSize);
    item.width += glyph.advance;
    item.height = (std::max)(item.height, glyph.ascent + glyph.descent);
    item.baselineOffset = (std::max)(item.baselineOffset, glyph.ascent);
    item.usedFontFallback = item.usedFontFallback || glyph.fromFallback;
    item.usedPlaceholderGlyphs = item.usedPlaceholderGlyphs || glyph.placeholder;
  }

  item.width = (std::max)(item.width, 1);
  item.height = (std::max)(item.height, 1);
  item.origin = {
    anchor.x + 4,
    anchor.y - item.height / 2};

  return item;
}

void TextLabelRenderer::render(const LabelItem &label, RhiRenderBackend &backend) const
{
  int cursorX = label.origin.x;
  const int baselineY = label.origin.y + label.baselineOffset;

  for(const auto codePoint : label.glyphText) {
    const auto &glyph = m_glyphCache.glyphFor(codePoint, label.pixelSize);
    if(!glyph.valid()) {
      cursorX += glyph.advance;
      continue;
    }

    if(glyph.width <= 0 || glyph.height <= 0 || glyph.alphaMask.empty()) {
      cursorX += glyph.advance;
      continue;
    }

    const auto expectedMaskSize = static_cast<std::size_t>(glyph.width * glyph.height);
    if(glyph.alphaMask.size() < expectedMaskSize) {
      cursorX += glyph.advance;
      continue;
    }

    const int glyphOriginX = cursorX + glyph.left;
    const int glyphOriginY = baselineY + glyph.top;
    for(int y = 0; y < glyph.height; ++y) {
      for(int x = 0; x < glyph.width; ++x) {
        const auto maskIndex = static_cast<std::size_t>(y * glyph.width + x);
        if(maskIndex >= glyph.alphaMask.size()) {
          continue;
        }

        const auto alpha = glyph.alphaMask[maskIndex];
        if(alpha == 0U) {
          continue;
        }

        backend.drawPoint(
          {glyphOriginX + x, glyphOriginY + y},
          0,
          label.color);
      }
    }

    cursorX += glyph.advance;
  }
}

}// namespace chart_view::runtime
