#ifndef CHART_VIEW_RUNTIME_GLYPH_CACHE_HPP
#define CHART_VIEW_RUNTIME_GLYPH_CACHE_HPP

#include "font_fallback.hpp"

#include <QFontMetrics>
#include <QImage>
#include <QList>
#include <QPainter>
#include <QPointF>
#include <QRectF>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace chart_view::runtime::text {

struct CachedGlyph
{
  char32_t codePoint{U'\0'};
  std::string family;
  int width{0};
  int height{0};
  int left{0};
  int top{0};
  int advance{0};
  int ascent{0};
  int descent{0};
  bool fromFallback{false};
  bool placeholder{false};
  std::vector<std::uint8_t> alphaMask;

  [[nodiscard]] bool valid() const noexcept
  {
    const auto expectedMaskSize = static_cast<std::size_t>((std::max)(width, 0) * (std::max)(height, 0));
    return advance > 0 && alphaMask.size() == expectedMaskSize;
  }
};

struct GlyphCacheStats
{
  std::size_t hits{0};
  std::size_t misses{0};
};

class GlyphCache
{
public:
  GlyphCache() = default;

  explicit GlyphCache(FontFallbackResolver resolver)
    : m_resolver(std::move(resolver))
  {
  }

  [[nodiscard]] const CachedGlyph &glyphFor(
    char32_t codePoint,
    std::uint32_t pixelSize) const
  {
    const GlyphKey key{codePoint, pixelSize};
    const auto it = m_cache.find(key);
    if(it != m_cache.end()) {
      ++m_hits;
      return it->second;
    }

    ++m_misses;
    auto inserted = m_cache.emplace(key, buildGlyph(codePoint, pixelSize));
    return inserted.first->second;
  }

  [[nodiscard]] GlyphCacheStats stats() const noexcept
  {
    return {m_hits, m_misses};
  }

private:
  struct GlyphKey
  {
    char32_t codePoint{U'\0'};
    std::uint32_t pixelSize{0U};

    [[nodiscard]] bool operator==(const GlyphKey &other) const noexcept
    {
      return codePoint == other.codePoint && pixelSize == other.pixelSize;
    }
  };

  struct GlyphKeyHasher
  {
    [[nodiscard]] std::size_t operator()(const GlyphKey &key) const noexcept
    {
      const auto codePointHash = std::hash<char32_t>{}(key.codePoint);
      const auto pixelSizeHash = std::hash<std::uint32_t>{}(key.pixelSize);
      return codePointHash ^ (pixelSizeHash << 1U);
    }
  };

  [[nodiscard]] static CachedGlyph buildPlaceholder(
    char32_t codePoint,
    std::uint32_t pixelSize)
  {
    constexpr std::array<std::string_view, 5> kRows{
      "###",
      "..#",
      ".#.",
      "...",
      ".#.",
    };

    const int scale = (std::max)(1, static_cast<int>(pixelSize / 6U));

    CachedGlyph glyph;
    glyph.codePoint = codePoint;
    glyph.family = "placeholder";
    glyph.width = static_cast<int>(kRows.front().size()) * scale;
    glyph.height = static_cast<int>(kRows.size()) * scale;
    glyph.left = 0;
    glyph.top = -glyph.height;
    glyph.advance = glyph.width + scale;
    glyph.ascent = glyph.height;
    glyph.descent = 0;
    glyph.placeholder = true;
    glyph.alphaMask.assign(static_cast<std::size_t>(glyph.width * glyph.height), 0U);

    for(std::size_t y = 0; y < kRows.size(); ++y) {
      for(std::size_t x = 0; x < kRows[y].size(); ++x) {
        if(kRows[y][x] != '#') {
          continue;
        }

        for(int dy = 0; dy < scale; ++dy) {
          for(int dx = 0; dx < scale; ++dx) {
            const auto px = static_cast<int>(x) * scale + dx;
            const auto py = static_cast<int>(y) * scale + dy;
            glyph.alphaMask[static_cast<std::size_t>(py * glyph.width + px)] = 255U;
          }
        }
      }
    }

    return glyph;
  }

  [[nodiscard]] CachedGlyph buildGlyph(
    char32_t codePoint,
    std::uint32_t pixelSize) const
  {
    const auto resolved = m_resolver.resolve(codePoint, pixelSize);
    if(!resolved.has_value() || !resolved->valid()) {
      return buildPlaceholder(codePoint, pixelSize);
    }

    const QString glyphString = QString::fromUcs4(&codePoint, 1);
    if(glyphString.isEmpty()) {
      return buildPlaceholder(codePoint, pixelSize);
    }

    const QFontMetrics metrics(resolved->font);
    const QRect inkRect = metrics.tightBoundingRect(glyphString);

    CachedGlyph glyph;
    glyph.codePoint = codePoint;
    glyph.family = resolved->family;
    glyph.width = (std::max)(inkRect.width(), 0);
    glyph.height = (std::max)(inkRect.height(), 0);
    glyph.left = inkRect.left();
    glyph.top = inkRect.top();
    glyph.advance = (std::max)(1, metrics.horizontalAdvance(glyphString));
    glyph.ascent = (std::max)(1, metrics.ascent());
    glyph.descent = (std::max)(0, metrics.descent());
    glyph.fromFallback = resolved->fromFallback;
    glyph.alphaMask.resize(static_cast<std::size_t>(glyph.width * glyph.height), 0U);

    if(glyph.width == 0 || glyph.height == 0) {
      return glyph;
    }

    QImage image(glyph.width, glyph.height, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    {
      QPainter painter(&image);
      painter.setRenderHint(QPainter::TextAntialiasing, true);
      painter.setPen(Qt::white);
      painter.setFont(resolved->font);
      painter.drawText(-inkRect.left(), -inkRect.top(), glyphString);
    }

    bool hasInk = false;
    for(int y = 0; y < glyph.height; ++y) {
      const auto *row = reinterpret_cast<const QRgb *>(image.constScanLine(y));
      for(int x = 0; x < glyph.width; ++x) {
        const auto alpha = static_cast<std::uint8_t>(qAlpha(row[x]));
        glyph.alphaMask[static_cast<std::size_t>(y * glyph.width + x)] = alpha;
        hasInk = hasInk || alpha != 0U;
      }
    }

    if(!hasInk) {
      return buildPlaceholder(codePoint, pixelSize);
    }

    if(!glyph.valid()) {
      return buildPlaceholder(codePoint, pixelSize);
    }

    return glyph;
  }

  FontFallbackResolver m_resolver;
  mutable std::size_t m_hits{0};
  mutable std::size_t m_misses{0};
  mutable std::unordered_map<GlyphKey, CachedGlyph, GlyphKeyHasher> m_cache;
};

}// namespace chart_view::runtime::text

#endif
