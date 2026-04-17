#include "line_symbol_renderer.hpp"
#include "portrayal/s52_presentation_assets.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace chart_view::runtime {

namespace {

struct PatternSpan
{
  bool draw{false};
  double length{0.0};
};

struct LinePattern
{
  std::string_view assetId;
  std::string_view styleKey;
  std::span<const PatternSpan> spans;
};

constexpr std::array<PatternSpan, 2> kDepthContourPattern{{
  {true, 12.0},
  {false, 6.0},
}};

constexpr std::array<PatternSpan, 4> kCoastlinePattern{{
  {true, 14.0},
  {false, 4.0},
  {true, 3.0},
  {false, 4.0},
}};

constexpr std::array<PatternSpan, 4> kChannelPattern{{
  {true, 8.0},
  {false, 4.0},
  {true, 2.0},
  {false, 4.0},
}};

constexpr std::array<LinePattern, 3> kPatterns{{
  {"DEPCN01", "line/depth_contour", kDepthContourPattern},
  {"COALNE01", "line/coastline", kCoastlinePattern},
  {"FAIRWY01", "line/channel", kChannelPattern},
}};

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

const LinePattern *findPattern(std::string_view assetId, std::string_view styleKey) noexcept
{
  const auto normalizedAssetId = normalizeStyleKey(assetId);
  if(!normalizedAssetId.empty()) {
    for(const auto &pattern : kPatterns) {
      if(normalizedAssetId == normalizeStyleKey(pattern.assetId)) {
        return &pattern;
      }
    }
  }

  const auto normalized = normalizeStyleKey(styleKey);
  for(const auto &pattern : kPatterns) {
    if(normalized == pattern.styleKey) {
      return &pattern;
    }
  }

  return nullptr;
}

const portrayal::S52PresentationAssets &presentationAssets()
{
  static const auto *assets = new portrayal::S52PresentationAssets();
  return *assets;
}

std::vector<PatternSpan> makeSyntheticPattern(std::string_view assetId, int thickness)
{
  const auto normalized = normalizeStyleKey(assetId);
  if(normalized.rfind("ls_sold_", 0) == 0) {
    return {{true, static_cast<double>(std::max(12, thickness * 6))}};
  }

  if(normalized.rfind("ls_dash_", 0) == 0) {
    return {
      {true, static_cast<double>(std::max(8, thickness * 4))},
      {false, static_cast<double>(std::max(4, thickness * 2))}};
  }

  if(normalized.rfind("ls_dot_", 0) == 0 || normalized.rfind("ls_dott_", 0) == 0) {
    return {
      {true, static_cast<double>(std::max(2, thickness))},
      {false, static_cast<double>(std::max(5, thickness * 3))}};
  }

  return {};
}

std::vector<PatternSpan> makeMetadataPattern(
  const portrayal::S52LineStyleAsset &asset,
  std::string_view styleKey,
  int thickness)
{
  const auto normalizedStyleKey = normalizeStyleKey(styleKey);
  const auto primaryDrawLength = std::clamp(asset.vectorMetrics.width / 250, 8, 18);
  const auto secondaryDrawLength = std::clamp(asset.vectorMetrics.width / 900, 2, 6);
  const auto gapLength = std::clamp(asset.vectorMetrics.height / 125, 4, 10);

  if(normalizedStyleKey == "line/coastline") {
    return {
      {true, static_cast<double>(primaryDrawLength + 2)},
      {false, static_cast<double>(gapLength)},
      {true, static_cast<double>(secondaryDrawLength)},
      {false, static_cast<double>(gapLength)}};
  }

  if(normalizedStyleKey == "line/depth_contour") {
    return {
      {true, static_cast<double>(primaryDrawLength)},
      {false, static_cast<double>(gapLength)}};
  }

  if(asset.hpgl.empty()) {
    return {};
  }

  return {
    {true, static_cast<double>(std::max(primaryDrawLength, thickness * 4))},
    {false, static_cast<double>(gapLength)}};
}

std::vector<PatternSpan> resolvePatternSpans(
  std::string_view assetId,
  std::string_view styleKey,
  int thickness)
{
  if(const auto *pattern = findPattern(assetId, styleKey); pattern != nullptr) {
    return {pattern->spans.begin(), pattern->spans.end()};
  }

  if(const auto synthetic = makeSyntheticPattern(assetId, thickness); !synthetic.empty()) {
    return synthetic;
  }

  if(const auto *asset = presentationAssets().findLineStyle(assetId); asset != nullptr) {
    if(const auto metadataPattern = makeMetadataPattern(*asset, styleKey, thickness);
       !metadataPattern.empty()) {
      return metadataPattern;
    }
  }

  return {};
}

double segmentLength(SurfacePoint start, SurfacePoint end) noexcept
{
  const auto dx = static_cast<double>(end.x - start.x);
  const auto dy = static_cast<double>(end.y - start.y);
  return std::sqrt(dx * dx + dy * dy);
}

SurfacePoint interpolatePoint(
  SurfacePoint start,
  SurfacePoint end,
  double segmentPosition,
  double segmentLen) noexcept
{
  if(segmentLen <= 1e-6) {
    return start;
  }

  const auto t = std::clamp(segmentPosition / segmentLen, 0.0, 1.0);
  return {
    static_cast<int>(std::lround(
      static_cast<double>(start.x) + (static_cast<double>(end.x - start.x) * t))),
    static_cast<int>(std::lround(
      static_cast<double>(start.y) + (static_cast<double>(end.y - start.y) * t)))};
}

}// namespace

bool LineSymbolRenderer::render(
  std::string_view assetId,
  std::string_view styleKey,
  std::span<const SurfacePoint> points,
  const portrayal::LineStyleRule &rule,
  RhiRenderBackend &backend) const
{
  const auto spans = resolvePatternSpans(assetId, styleKey, rule.thickness);
  if(spans.empty() || points.size() < 2) {
    return false;
  }

  std::size_t patternIndex = 0;
  double spanOffset = 0.0;

  for(std::size_t i = 1; i < points.size(); ++i) {
    const auto start = points[i - 1];
    const auto end = points[i];
    const auto length = segmentLength(start, end);
    if(length <= 1e-6) {
      continue;
    }

    double segmentOffset = 0.0;
    while(segmentOffset < length) {
      const auto &span = spans[patternIndex];
      const auto spanRemaining = span.length - spanOffset;
      const auto step = std::min(spanRemaining, length - segmentOffset);
      if(span.draw && step > 0.0) {
        const auto drawStart = interpolatePoint(start, end, segmentOffset, length);
        const auto drawEnd = interpolatePoint(start, end, segmentOffset + step, length);
        backend.drawLine(drawStart, drawEnd, rule.thickness, rule.color);
      }

      segmentOffset += step;
      spanOffset += step;
      if(spanOffset + 1e-6 >= span.length) {
        spanOffset = 0.0;
        patternIndex = (patternIndex + 1U) % spans.size();
      }
    }
  }

  return true;
}

}// namespace chart_view::runtime
