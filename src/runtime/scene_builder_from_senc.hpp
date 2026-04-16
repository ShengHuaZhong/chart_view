#ifndef CHART_VIEW_RUNTIME_SCENE_BUILDER_FROM_SENC_HPP
#define CHART_VIEW_RUNTIME_SCENE_BUILDER_FROM_SENC_HPP

#include "scene_snapshot.hpp"
#include "viewport_state.hpp"
#include "chart_data/feature_chart_dataset.hpp"
#include "chart_data/geometry.hpp"
#include "quilt/quilt_plan.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <span>

namespace chart_view::runtime {

// Builds a SceneSnapshot from a decoded SENC dataset and a viewport.
// Phase 1: simple bounding-box visibility filter (no spatial index).
class SceneBuilderFromSenc
{
public:
  SceneBuilderFromSenc() = default;

  [[nodiscard]] std::shared_ptr<const SceneSnapshot> build(
    const quilt::QuiltPlan &plan,
    std::span<const chart_data::FeatureChartDataset> datasets,
    const ViewportState &viewport) const
  {
    SceneModel model;

    if(!viewport.isValid() || plan.empty() || datasets.empty()) {
      return std::make_shared<SceneSnapshot>(model, viewport);
    }

    const auto layerCount = std::min(plan.layers().size(), datasets.size());
    for(std::size_t i = 0; i < layerCount; ++i) {
      const auto &layer = plan.layers()[i];
      const auto &dataset = datasets[i];
      if(dataset.empty()) {
        continue;
      }

      const auto sourceChartIndex = model.addChart(
        SceneChartEntry{layer.chartId, layer.sourceType, layer.drawOrder});
      const auto visibleExtent = layer.visibleExtent.isValid() ? layer.visibleExtent : layer.fullExtent;
      appendVisibleFeatures(model, dataset, sourceChartIndex, visibleExtent);
    }

    return std::make_shared<SceneSnapshot>(model, viewport);
  }

  // Build a scene snapshot containing only features visible in the viewport.
  [[nodiscard]] std::shared_ptr<const SceneSnapshot> build(
    const chart_data::FeatureChartDataset &dataset,
    const ViewportState &viewport) const
  {
    SceneModel model;

    if (!viewport.isValid() || dataset.empty()) {
      return std::make_shared<SceneSnapshot>(model, viewport);
    }

    const auto sourceChartIndex = model.addChart(
      SceneChartEntry{dataset.meta().name, dataset.meta().sourceType, 0});
    appendVisibleFeatures(
      model,
      dataset,
      sourceChartIndex,
      computeViewportExtent(viewport.viewport()));

    return std::make_shared<SceneSnapshot>(model, viewport);
  }

  // Build a scene snapshot containing ALL features (no viewport culling).
  [[nodiscard]] std::shared_ptr<const SceneSnapshot> buildAll(
    const chart_data::FeatureChartDataset &dataset,
    const ViewportState &viewport) const
  {
    SceneModel model;

    if(!dataset.empty()) {
      const auto sourceChartIndex = model.addChart(
        SceneChartEntry{dataset.meta().name, dataset.meta().sourceType, 0});
      appendAllFeatures(model, dataset, sourceChartIndex);
    }

    return std::make_shared<SceneSnapshot>(model, viewport);
  }

private:
  // Compute the geographic extent of a viewport using a simple equirectangular
  // approximation.  Good enough for Phase 1 visibility culling.
  [[nodiscard]] static chart_data::Extent computeViewportExtent(
    const chart_view_viewport_t &vp) noexcept
  {
    // Approximate metres-per-degree at the viewport centre latitude.
    constexpr double kMetresPerDegLat = 111320.0;
    const double cosLat = std::cos(vp.center_lat * 3.14159265358979323846 / 180.0);
    const double metresPerDegLon = kMetresPerDegLat * (cosLat > 1e-6 ? cosLat : 1e-6);

    // Assume 96 DPI, approx 3780 pixels/metre.
    constexpr double kPixelsPerMetre = 3779.5275591;

    const double halfWidthDeg =
      (vp.pixel_width / 2.0) / kPixelsPerMetre * vp.scale_denominator / metresPerDegLon;
    const double halfHeightDeg =
      (vp.pixel_height / 2.0) / kPixelsPerMetre * vp.scale_denominator / kMetresPerDegLat;

    return {
      vp.center_lon - halfWidthDeg,
      vp.center_lat - halfHeightDeg,
      vp.center_lon + halfWidthDeg,
      vp.center_lat + halfHeightDeg};
  }

  // Compute the bounding box of a feature's geometry.
  [[nodiscard]] static chart_data::Extent computeFeatureExtent(
    const chart_data::Geometry &geom) noexcept
  {
    chart_data::Extent ext{1e30, 1e30, -1e30, -1e30};

    auto expand = [&](const chart_data::Coordinate &c) {
      if (c.lon < ext.minLon) ext.minLon = c.lon;
      if (c.lon > ext.maxLon) ext.maxLon = c.lon;
      if (c.lat < ext.minLat) ext.minLat = c.lat;
      if (c.lat > ext.maxLat) ext.maxLat = c.lat;
    };

    std::visit(
      [&](auto &&g) {
        using T = std::decay_t<decltype(g)>;
        if constexpr (std::is_same_v<T, chart_data::PointGeometry>) {
          expand(g.position);
        } else if constexpr (std::is_same_v<T, chart_data::LineGeometry>) {
          for (const auto &v : g.vertices) expand(v);
        } else if constexpr (std::is_same_v<T, chart_data::AreaGeometry>) {
          for (const auto &v : g.exteriorRing) expand(v);
        }
      },
      geom);

    return ext;
  }

  // Simple AABB overlap test.
  [[nodiscard]] static bool extentsOverlap(
    const chart_data::Extent &a,
    const chart_data::Extent &b) noexcept
  {
    return a.minLon <= b.maxLon && a.maxLon >= b.minLon
        && a.minLat <= b.maxLat && a.maxLat >= b.minLat;
  }

  static void appendVisibleFeatures(
    SceneModel &model,
    const chart_data::FeatureChartDataset &dataset,
    std::uint32_t sourceChartIndex,
    const chart_data::Extent &visibleExtent)
  {
    const auto &features = dataset.features();
    for(std::uint32_t i = 0; i < static_cast<std::uint32_t>(features.size()); ++i) {
      const auto featureBbox = computeFeatureExtent(features[i].geometry);
      if(!extentsOverlap(visibleExtent, featureBbox)) {
        continue;
      }

      SceneLayerEntry entry;
      entry.sourceChartIndex = sourceChartIndex;
      entry.layerId = features[i].classCode;
      entry.featureIndex = i;
      entry.geometryType = static_cast<std::uint8_t>(chart_data::geometryType(features[i].geometry));
      model.addLayer(entry);
    }
  }

  static void appendAllFeatures(
    SceneModel &model,
    const chart_data::FeatureChartDataset &dataset,
    std::uint32_t sourceChartIndex)
  {
    const auto &features = dataset.features();
    for(std::uint32_t i = 0; i < static_cast<std::uint32_t>(features.size()); ++i) {
      SceneLayerEntry entry;
      entry.sourceChartIndex = sourceChartIndex;
      entry.layerId = features[i].classCode;
      entry.featureIndex = i;
      entry.geometryType = static_cast<std::uint8_t>(chart_data::geometryType(features[i].geometry));
      model.addLayer(entry);
    }
  }
};

}// namespace chart_view::runtime

#endif
