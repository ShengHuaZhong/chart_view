#ifndef CHART_VIEW_RUNTIME_FONT_FALLBACK_HPP
#define CHART_VIEW_RUNTIME_FONT_FALLBACK_HPP

#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace chart_view::runtime::text {

struct ResolvedFont
{
  std::string family;
  QFont font;
  bool fromFallback{false};

  [[nodiscard]] bool valid() const noexcept
  {
    return !family.empty();
  }
};

class FontFallbackResolver
{
public:
  FontFallbackResolver()
    : m_preferredFamilies(defaultFamiliesUtf8())
  {
  }

  explicit FontFallbackResolver(
    std::vector<std::string> preferredFamilies,
    bool includeInstalledFallbacks = true)
    : m_preferredFamilies(std::move(preferredFamilies))
    , m_includeInstalledFallbacks(includeInstalledFallbacks)
  {
    if(m_preferredFamilies.empty()) {
      m_preferredFamilies = defaultFamiliesUtf8();
    }
  }

  [[nodiscard]] static std::vector<std::string> defaultFamiliesUtf8()
  {
    std::vector<std::string> families;

    const auto appendFamily = [&](const QString &family) {
      if(family.isEmpty()) {
        return;
      }

      const auto utf8 = family.toUtf8().toStdString();
      if(utf8.empty()
         || std::find(families.begin(), families.end(), utf8) != families.end()) {
        return;
      }
      families.push_back(utf8);
    };

    appendFamily(QFontDatabase::systemFont(QFontDatabase::GeneralFont).family());
    appendFamily(QStringLiteral("Segoe UI"));
    appendFamily(QStringLiteral("Microsoft YaHei UI"));
    appendFamily(QStringLiteral("Microsoft YaHei"));
    appendFamily(QStringLiteral("SimSun"));
    appendFamily(QStringLiteral("Yu Gothic UI"));
    appendFamily(QStringLiteral("Malgun Gothic"));
    appendFamily(QStringLiteral("Nirmala UI"));
    appendFamily(QStringLiteral("Segoe UI Symbol"));
    appendFamily(QStringLiteral("Segoe UI Emoji"));
    appendFamily(QStringLiteral("Arial Unicode MS"));

    return families;
  }

  [[nodiscard]] static std::string pickAvailableFamily(
    std::initializer_list<std::string_view> candidates)
  {
    const QFontDatabase database;
    const auto installed = database.families();

    for(const auto candidate : candidates) {
      const QString family = QString::fromUtf8(candidate.data(), static_cast<qsizetype>(candidate.size()));
      if(installed.contains(family)) {
        return family.toUtf8().toStdString();
      }
    }

    return {};
  }

  [[nodiscard]] std::optional<ResolvedFont> resolve(
    char32_t codePoint,
    std::uint32_t pixelSize) const
  {
    const GlyphLookupKey key{codePoint, pixelSize};
    const auto cacheIt = m_resolutionCache.find(key);
    if(cacheIt != m_resolutionCache.end()) {
      return cacheIt->second;
    }

    if(codePoint == U'\0') {
      return std::nullopt;
    }

    const auto &candidates = candidateFamilies();
    for(std::size_t index = 0; index < candidates.size(); ++index) {
      const auto resolved = resolveFamily(candidates[index], codePoint, pixelSize, index != 0U);
      if(resolved.has_value()) {
        m_resolutionCache.emplace(key, resolved);
        return resolved;
      }
    }

    m_resolutionCache.emplace(key, std::nullopt);
    return std::nullopt;
  }

private:
  struct GlyphLookupKey
  {
    char32_t codePoint{U'\0'};
    std::uint32_t pixelSize{0U};

    [[nodiscard]] bool operator==(const GlyphLookupKey &other) const noexcept
    {
      return codePoint == other.codePoint && pixelSize == other.pixelSize;
    }
  };

  struct GlyphLookupKeyHasher
  {
    [[nodiscard]] std::size_t operator()(const GlyphLookupKey &key) const noexcept
    {
      const auto codePointHash = std::hash<char32_t>{}(key.codePoint);
      const auto pixelSizeHash = std::hash<std::uint32_t>{}(key.pixelSize);
      return codePointHash ^ (pixelSizeHash << 1U);
    }
  };

  [[nodiscard]] const std::vector<std::string> &candidateFamilies() const
  {
    if(!m_candidateFamilies.empty()) {
      return m_candidateFamilies;
    }

    m_candidateFamilies = m_preferredFamilies;
    if(!m_includeInstalledFallbacks) {
      return m_candidateFamilies;
    }

    const QFontDatabase database;
    for(const auto &family : database.families()) {
      const auto utf8 = family.toUtf8().toStdString();
      if(std::find(m_candidateFamilies.begin(), m_candidateFamilies.end(), utf8)
         == m_candidateFamilies.end()) {
        m_candidateFamilies.push_back(std::move(utf8));
      }
    }

    return m_candidateFamilies;
  }

  [[nodiscard]] static std::optional<ResolvedFont> resolveFamily(
    const std::string &familyUtf8,
    char32_t codePoint,
    std::uint32_t pixelSize,
    bool fromFallback)
  {
    const QString family = QString::fromUtf8(familyUtf8);
    if(family.isEmpty()) {
      return std::nullopt;
    }

    QFont font(family);
    font.setPixelSize((std::max)(static_cast<int>(pixelSize), 1));
    font.setHintingPreference(QFont::PreferNoHinting);
    font.setStyleStrategy(static_cast<QFont::StyleStrategy>(
      QFont::PreferDefault | QFont::PreferQuality));

    const QString glyphString = QString::fromUcs4(&codePoint, 1);
    if(glyphString.isEmpty()) {
      return std::nullopt;
    }

    const QFontMetrics metrics(font);
    if(glyphString.trimmed().isEmpty()) {
      ResolvedFont resolved;
      resolved.family = familyUtf8;
      resolved.font = font;
      resolved.fromFallback = fromFallback;
      return resolved;
    }

    const int imageWidth = (std::max)(metrics.horizontalAdvance(glyphString) + pixelPadding(pixelSize), 1);
    const int imageHeight = (std::max)(metrics.height() + pixelPadding(pixelSize), 1);

    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    {
      QPainter painter(&image);
      painter.setRenderHint(QPainter::TextAntialiasing, true);
      painter.setPen(Qt::white);
      painter.setFont(font);
      painter.drawText(pixelPadding(pixelSize) / 2, metrics.ascent(), glyphString);
    }

    if(!imageHasInk(image)) {
      return std::nullopt;
    }

    ResolvedFont resolved;
    resolved.family = familyUtf8;
    resolved.font = font;
    resolved.fromFallback = fromFallback;
    return resolved;
  }

  [[nodiscard]] static int pixelPadding(std::uint32_t pixelSize) noexcept
  {
    return (std::max)(2, static_cast<int>(pixelSize / 2U));
  }

  [[nodiscard]] static bool imageHasInk(const QImage &image)
  {
    for(int y = 0; y < image.height(); ++y) {
      const auto *row = reinterpret_cast<const QRgb *>(image.constScanLine(y));
      for(int x = 0; x < image.width(); ++x) {
        if(qAlpha(row[x]) != 0U) {
          return true;
        }
      }
    }

    return false;
  }

  std::vector<std::string> m_preferredFamilies;
  bool m_includeInstalledFallbacks{true};
  mutable std::vector<std::string> m_candidateFamilies;
  mutable std::unordered_map<GlyphLookupKey, std::optional<ResolvedFont>, GlyphLookupKeyHasher>
    m_resolutionCache;
};

}// namespace chart_view::runtime::text

#endif
