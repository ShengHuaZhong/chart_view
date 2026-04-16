#ifndef CHART_VIEW_RUNTIME_FEATURE_LAYER_RENDERER_HPP
#define CHART_VIEW_RUNTIME_FEATURE_LAYER_RENDERER_HPP

#include "rhi_render_backend.hpp"
#include "scene_snapshot.hpp"
#include "area_symbol_renderer.hpp"
#include "chart_data/feature_chart_dataset.hpp"
#include "chart_data/geometry.hpp"
#include "label_layout.hpp"
#include "line_symbol_renderer.hpp"
#include "point_symbol_renderer.hpp"
#include "projection/projection_context.hpp"
#include "projection/projected_viewport.hpp"
#include "text_label_renderer.hpp"
#include "portrayal/feature_symbolizer.hpp"
#include "portrayal/portrayal_registry.hpp"

#include <chart_view/runtime/chart_runtime_types.h>

#include <cmath>
#include <cstdint>
#include <span>
#include <vector>

namespace chart_view::runtime {

// A 2D vertex used by the feature layer renderer.
struct FeatureVertex
{
  float x{0.0F};
  float y{0.0F};
  float r{0.0F};
  float g{0.0F};
  float b{0.0F};
  float a{1.0F};
};

// Result of a single render frame.
struct FeatureRenderResult
{
  chart_view_status_t status{chart_view_status_ok};
  std::uint32_t pointsRendered{0};
  std::uint32_t linesRendered{0};
  std::uint32_t areasRendered{0};
  std::uint32_t totalVertices{0};
};

// Renders chart features (point / line / area) from a SceneSnapshot.
//
// Phase 1 draws minimum visible geometry into a runtime-owned RGBA frame
// buffer while preserving the runtime-side render ownership boundary.
class FeatureLayerRenderer
{
public:
  FeatureLayerRenderer();
  explicit FeatureLayerRenderer(portrayal::S52DisplaySettings settings);

  // Internal viewport mapping used by the geometry render path.
  struct ViewportProjection
  {
    double centerLon{0.0};
    double centerLat{0.0};
    double scaleX{1.0};
    double scaleY{1.0};
    int pixelWidth{1};
    int pixelHeight{1};

    [[nodiscard]] SurfacePoint projectToPixel(const chart_data::Coordinate &coord) const noexcept;
  };

  [[nodiscard]] portrayal::PortrayalRegistry &portrayalRegistry() noexcept { return m_portrayal; }
  [[nodiscard]] const portrayal::PortrayalRegistry &portrayalRegistry() const noexcept
  {
    return m_portrayal;
  }

  [[nodiscard]] FeatureRenderResult render(
    const SceneSnapshot &snapshot,
    std::span<const chart_data::FeatureChartDataset> datasets,
    RhiRenderBackend &backend) const;

  // Render features referenced by the snapshot, looking up geometry from dataset.
  [[nodiscard]] FeatureRenderResult render(
    const SceneSnapshot &snapshot,
    const chart_data::FeatureChartDataset &dataset,
    RhiRenderBackend &backend) const;

private:
  [[nodiscard]] static ViewportProjection makeProjection(
    const chart_view_viewport_t &vp) noexcept;

  void renderFeature(
    const chart_data::Feature &feature,
    const portrayal::FeatureSymbolization &symbolization,
    const ViewportProjection &proj,
    RhiRenderBackend &backend,
    FeatureRenderResult &result) const;
  void renderFeatureLabel(
    const chart_data::Feature &feature,
    const portrayal::FeatureSymbolization &symbolization,
    const ViewportProjection &proj,
    const projection::ProjectionContext *labelProjectionContext,
    const projection::ProjectedViewport *labelViewport,
    std::vector<label::LabelBounds> &occupiedLabelBounds,
    RhiRenderBackend &backend) const;
  portrayal::PortrayalRegistry m_portrayal;
  portrayal::FeatureSymbolizer m_symbolizer;
  AreaSymbolRenderer m_areaSymbols;
  LineSymbolRenderer m_lineSymbols;
  PointSymbolRenderer m_pointSymbols;
  TextLabelRenderer m_textLabels;
};

}// namespace chart_view::runtime

#endif
