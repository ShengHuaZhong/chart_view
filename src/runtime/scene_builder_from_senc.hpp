#ifndef CHART_VIEW_RUNTIME_SCENE_BUILDER_FROM_SENC_HPP
#define CHART_VIEW_RUNTIME_SCENE_BUILDER_FROM_SENC_HPP

#include "scene_snapshot.hpp"
#include "viewport_state.hpp"
#include "chart_data/feature_chart_dataset.hpp"
#include "chart_data/geometry.hpp"
#include "projection/projected_bounds.hpp"
#include "quilt/quilt_plan.hpp"

#include <algorithm>
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

    const auto projectionContext = projection::ProjectionContext::createMercator();
    if(!projectionContext.isValid()) {
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
      projection::ProjectedExtent visibleProjectedExtent{};
      if(!projectionContext.projectExtent(visibleExtent, visibleProjectedExtent)) {
        continue;
      }

      appendVisibleFeatures(
        model,
        dataset,
        sourceChartIndex,
        projectionContext,
        visibleProjectedExtent);
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

    const auto projectionContext = projection::ProjectionContext::createMercator();
    if(!projectionContext.isValid()) {
      return std::make_shared<SceneSnapshot>(model, viewport);
    }

    projection::ProjectedExtent visibleProjectedExtent{};
    if(!projection::projectViewportBounds(
         viewport.viewport(),
         projectionContext,
         visibleProjectedExtent)) {
      return std::make_shared<SceneSnapshot>(model, viewport);
    }

    const auto sourceChartIndex = model.addChart(
      SceneChartEntry{dataset.meta().name, dataset.meta().sourceType, 0});
    appendVisibleFeatures(
      model,
      dataset,
      sourceChartIndex,
      projectionContext,
      visibleProjectedExtent);

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
  static void appendVisibleFeatures(
    SceneModel &model,
    const chart_data::FeatureChartDataset &dataset,
    std::uint32_t sourceChartIndex,
    const projection::ProjectionContext &projectionContext,
    const projection::ProjectedExtent &visibleExtent)
  {
    const auto &features = dataset.features();
    for(std::uint32_t i = 0; i < static_cast<std::uint32_t>(features.size()); ++i) {
      projection::ProjectedExtent featureBbox{};
      if(!projection::projectGeometryExtent(
           projectionContext,
           features[i].geometry,
           featureBbox)
         || !projection::projectedExtentsOverlap(visibleExtent, featureBbox)) {
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
