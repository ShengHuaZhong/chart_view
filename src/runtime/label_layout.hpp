#ifndef CHART_VIEW_RUNTIME_LABEL_LAYOUT_HPP
#define CHART_VIEW_RUNTIME_LABEL_LAYOUT_HPP

#include "chart_data/feature.hpp"
#include "projection/projected_bounds.hpp"
#include "render_types.hpp"
#include "unicode_text.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace chart_view::runtime::label {

struct SelectedLabelText
{
  text::UnicodeText text;
  std::string sourceAttribute;
  bool preferredNationalName{false};
};

struct LabelBounds
{
  int left{0};
  int top{0};
  int right{0};
  int bottom{0};

  [[nodiscard]] bool isValid() const noexcept
  {
    return left <= right && top <= bottom;
  }
};

inline constexpr std::size_t kMaxLabelLength = 24U;

inline const std::string *findStringAttribute(
  const chart_data::Feature &feature,
  std::string_view key) noexcept
{
  const auto it = feature.attributes.find(std::string(key));
  if(it == feature.attributes.end()) {
    return nullptr;
  }

  return std::get_if<std::string>(&it->second);
}

inline bool hasRenderableCodePoint(const text::UnicodeText &value) noexcept
{
  return std::any_of(
    value.codePoints.begin(),
    value.codePoints.end(),
    [](char32_t codePoint) {
      return codePoint != text::detail::kReplacementCodePoint;
    });
}

inline std::optional<SelectedLabelText> selectMultilingualLabelText(
  const chart_data::Feature &feature,
  std::size_t maxCodePoints = kMaxLabelLength)
{
  struct Candidate
  {
    std::string_view key;
    bool preferredNationalName;
  };

  constexpr std::array<Candidate, 2> kCandidates{{
    {"NOBJNM", true},
    {"OBJNAM", false},
  }};

  for(const auto &candidate : kCandidates) {
    const auto *value = findStringAttribute(feature, candidate.key);
    if(value == nullptr || value->empty()) {
      continue;
    }

    auto decoded = text::decodeUtf8(*value, maxCodePoints);
    if(decoded.empty() || !hasRenderableCodePoint(decoded)) {
      continue;
    }

    return SelectedLabelText{
      std::move(decoded),
      std::string(candidate.key),
      candidate.preferredNationalName};
  }

  return std::nullopt;
}

inline bool projectedPointToPixel(
  const projection::ProjectedViewport &viewport,
  const projection::ProjectedPoint &projected,
  SurfacePoint &out) noexcept
{
  if(!viewport.isValid() || !projected.isFinite()) {
    return false;
  }

  const auto pixelX =
    static_cast<double>(viewport.pixelWidth) * 0.5 + (projected.x - viewport.center.x) / viewport.metresPerPixel;
  const auto pixelY =
    static_cast<double>(viewport.pixelHeight) * 0.5 - (projected.y - viewport.center.y) / viewport.metresPerPixel;
  if(!std::isfinite(pixelX) || !std::isfinite(pixelY)) {
    return false;
  }

  out = {
    static_cast<int>(std::lround(pixelX)),
    static_cast<int>(std::lround(pixelY))};
  return true;
}

inline bool resolveProjectedLabelAnchor(
  const chart_data::Feature &feature,
  const projection::ProjectionContext &projectionContext,
  const projection::ProjectedViewport &viewport,
  SurfacePoint &out) noexcept
{
  projection::ProjectedPoint projectedAnchor{};
  bool resolved = false;

  std::visit(
    [&](auto &&geometry) {
      using T = std::decay_t<decltype(geometry)>;

      if constexpr(std::is_same_v<T, chart_data::PointGeometry>) {
        resolved = projectionContext.project(geometry.position, projectedAnchor);
      } else if constexpr(std::is_same_v<T, chart_data::LineGeometry>
                          || std::is_same_v<T, chart_data::AreaGeometry>) {
        projection::ProjectedExtent geometryExtent{};
        resolved = projection::projectGeometryExtent(
          projectionContext,
          feature.geometry,
          geometryExtent);
        if(resolved) {
          projectedAnchor = {
            (geometryExtent.minX + geometryExtent.maxX) * 0.5,
            (geometryExtent.minY + geometryExtent.maxY) * 0.5};
        }
      }
    },
    feature.geometry);

  return resolved && projectedPointToPixel(viewport, projectedAnchor, out);
}

inline LabelBounds computeLabelBounds(
  SurfacePoint origin,
  int width,
  int height) noexcept
{
  const auto safeWidth = (std::max)(width, 1);
  const auto safeHeight = (std::max)(height, 1);
  return {
    origin.x,
    origin.y,
    origin.x + safeWidth - 1,
    origin.y + safeHeight - 1};
}

inline bool labelBoundsVisible(
  const LabelBounds &bounds,
  int pixelWidth,
  int pixelHeight) noexcept
{
  return bounds.isValid() && pixelWidth > 0 && pixelHeight > 0
      && bounds.right >= 0 && bounds.bottom >= 0
      && bounds.left < pixelWidth && bounds.top < pixelHeight;
}

inline bool labelBoundsOverlap(
  const LabelBounds &lhs,
  const LabelBounds &rhs,
  int padding = 2) noexcept
{
  if(!lhs.isValid() || !rhs.isValid()) {
    return false;
  }

  return (lhs.left - padding) <= (rhs.right + padding)
      && (lhs.right + padding) >= (rhs.left - padding)
      && (lhs.top - padding) <= (rhs.bottom + padding)
      && (lhs.bottom + padding) >= (rhs.top - padding);
}

}// namespace chart_view::runtime::label

#endif
