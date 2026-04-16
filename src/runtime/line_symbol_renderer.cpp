#include "line_symbol_renderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cctype>
#include <string>

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
  const auto *pattern = findPattern(assetId, styleKey);
  if(pattern == nullptr || points.size() < 2) {
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
      const auto &span = pattern->spans[patternIndex];
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
        patternIndex = (patternIndex + 1U) % pattern->spans.size();
      }
    }
  }

  return true;
}

}// namespace chart_view::runtime
