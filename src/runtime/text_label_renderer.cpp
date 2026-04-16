#include "text_label_renderer.hpp"

#include <algorithm>
#include <string_view>

namespace chart_view::runtime {

text::GlyphCache &TextLabelRenderer::glyphCache() const
{
  if(!m_glyphCache) {
    m_glyphCache = std::make_unique<text::GlyphCache>();
  }

  return *m_glyphCache;
}

std::optional<LabelItem> TextLabelRenderer::layout(
  std::string_view textKey,
  const chart_data::Feature &feature,
  SurfacePoint anchor,
  const portrayal::TextRule &rule) const
{
  if(textKey.empty()) {
    return std::nullopt;
  }

  auto selectedText = label::selectMultilingualLabelText(feature);
  if(!selectedText.has_value() || selectedText->text.empty()) {
    return std::nullopt;
  }

  LabelItem item;
  item.text = std::move(selectedText->text.utf8);
  item.sourceAttribute = std::move(selectedText->sourceAttribute);
  item.glyphText = std::move(selectedText->text.codePoints);
  item.anchor = anchor;
  item.color = rule.color;
  item.pixelSize = rule.pixelSize;
  item.width = 0;
  item.height = 0;
  item.baselineOffset = 0;
  item.preferredNationalName = selectedText->preferredNationalName;

  for(const auto codePoint : item.glyphText) {
    const auto &glyph = glyphCache().glyphFor(codePoint, rule.pixelSize);
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
  item.bounds = label::computeLabelBounds(item.origin, item.width, item.height);

  return item;
}

void TextLabelRenderer::render(const LabelItem &label, RhiRenderBackend &backend) const
{
  int cursorX = label.origin.x;
  const int baselineY = label.origin.y + label.baselineOffset;

  for(const auto codePoint : label.glyphText) {
    const auto &glyph = glyphCache().glyphFor(codePoint, label.pixelSize);
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
