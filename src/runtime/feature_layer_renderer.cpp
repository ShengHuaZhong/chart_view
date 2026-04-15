#include "feature_layer_renderer.hpp"

#include <algorithm>
#include <array>
#include <type_traits>
#include <variant>

namespace chart_view::runtime {

namespace {
constexpr SurfaceColor kBackgroundColor{230U, 230U, 217U, 255U};
constexpr SurfaceColor kPointColor{196U, 46U, 46U, 255U};
constexpr SurfaceColor kLineColor{24U, 38U, 55U, 255U};
constexpr SurfaceColor kAreaFillColor{162U, 201U, 229U, 204U};
constexpr SurfaceColor kAreaOutlineColor{44U, 91U, 134U, 255U};
constexpr int kPointRadius = 4;
constexpr int kLineThickness = 2;
constexpr int kAreaOutlineThickness = 1;
}// namespace

SurfacePoint FeatureLayerRenderer::ViewportProjection::projectToPixel(
  const chart_data::Coordinate &coord) const noexcept
{
  const double ndcX = (coord.lon - centerLon) * scaleX;
  const double ndcY = (coord.lat - centerLat) * scaleY;

  const auto x = static_cast<int>(std::lround((ndcX + 1.0) * 0.5 * static_cast<double>(pixelWidth - 1)));
  const auto y = static_cast<int>(
    std::lround((1.0 - (ndcY + 1.0) * 0.5) * static_cast<double>(pixelHeight - 1)));

  return {x, y};
}

FeatureLayerRenderer::ViewportProjection FeatureLayerRenderer::makeProjection(
  const chart_view_viewport_t &vp) noexcept
{
  ViewportProjection proj;
  proj.centerLon = vp.center_lon;
  proj.centerLat = vp.center_lat;
  proj.pixelWidth = std::max(vp.pixel_width, 1);
  proj.pixelHeight = std::max(vp.pixel_height, 1);

  constexpr double kMetresPerDegLat = 111320.0;
  const double cosLat = std::cos(vp.center_lat * 3.14159265358979323846 / 180.0);
  const double metresPerDegLon = kMetresPerDegLat * (cosLat > 1e-6 ? cosLat : 1e-6);

  constexpr double kPixelsPerMetre = 3779.5275591;

  const double halfWidthDeg =
    (vp.pixel_width / 2.0) / kPixelsPerMetre * vp.scale_denominator / metresPerDegLon;
  const double halfHeightDeg =
    (vp.pixel_height / 2.0) / kPixelsPerMetre * vp.scale_denominator / kMetresPerDegLat;

  // Map half-extent to NDC [-1, 1].
  proj.scaleX = (halfWidthDeg > 1e-12) ? (1.0 / halfWidthDeg) : 1.0;
  proj.scaleY = (halfHeightDeg > 1e-12) ? (1.0 / halfHeightDeg) : 1.0;

  return proj;
}

void FeatureLayerRenderer::renderFeature(
  const chart_data::Feature &feature,
  const ViewportProjection &proj,
  RhiRenderBackend &backend,
  FeatureRenderResult &result) const
{
  std::visit(
    [&](auto &&geom) {
      using T = std::decay_t<decltype(geom)>;

      if constexpr(std::is_same_v<T, chart_data::PointGeometry>) {
        backend.drawPoint(proj.projectToPixel(geom.position), kPointRadius, kPointColor);
        ++result.pointsRendered;
        ++result.totalVertices;

      } else if constexpr(std::is_same_v<T, chart_data::LineGeometry>) {
        std::vector<SurfacePoint> points;
        points.reserve(geom.vertices.size());
        for(const auto &vertex : geom.vertices) {
          points.push_back(proj.projectToPixel(vertex));
          ++result.totalVertices;
        }
        for(std::size_t i = 1; i < points.size(); ++i) {
          backend.drawLine(points[i - 1], points[i], kLineThickness, kLineColor);
        }
        ++result.linesRendered;

      } else if constexpr(std::is_same_v<T, chart_data::AreaGeometry>) {
        std::vector<SurfacePoint> exterior;
        exterior.reserve(geom.exteriorRing.size());
        for(const auto &vertex : geom.exteriorRing) {
          exterior.push_back(proj.projectToPixel(vertex));
          ++result.totalVertices;
        }
        backend.fillPolygon(exterior, kAreaFillColor);
        backend.drawClosedPolyline(exterior, kAreaOutlineThickness, kAreaOutlineColor);

        for(const auto &hole : geom.interiorRings) {
          std::vector<SurfacePoint> holePoints;
          holePoints.reserve(hole.size());
          for(const auto &vertex : hole) {
            holePoints.push_back(proj.projectToPixel(vertex));
          }
          backend.fillPolygon(holePoints, kBackgroundColor);
          backend.drawClosedPolyline(holePoints, kAreaOutlineThickness, kAreaOutlineColor);
        }
        ++result.areasRendered;
      }
    },
    feature.geometry);
}

FeatureRenderResult FeatureLayerRenderer::render(
  const SceneSnapshot &snapshot,
  const chart_data::FeatureChartDataset &dataset,
  RhiRenderBackend &backend) const
{
  FeatureRenderResult result;

  if (!backend.isInitialized()) {
    result.status = chart_view_status_not_initialized;
    return result;
  }

  if (snapshot.empty()) {
    (void)backend.renderClearFrame(
      static_cast<float>(kBackgroundColor[0]) / 255.0F,
      static_cast<float>(kBackgroundColor[1]) / 255.0F,
      static_cast<float>(kBackgroundColor[2]) / 255.0F,
      static_cast<float>(kBackgroundColor[3]) / 255.0F);
    return result;
  }

  auto clearStatus = backend.renderClearFrame(
    static_cast<float>(kBackgroundColor[0]) / 255.0F,
    static_cast<float>(kBackgroundColor[1]) / 255.0F,
    static_cast<float>(kBackgroundColor[2]) / 255.0F,
    static_cast<float>(kBackgroundColor[3]) / 255.0F);
  if(clearStatus != chart_view_status_ok) {
    result.status = clearStatus;
    return result;
  }

  const auto proj = makeProjection(snapshot.viewport());

  const auto &features = dataset.features();
  for(const auto &entry : snapshot.layers()) {
    if(entry.featureIndex >= features.size()) {
      continue;
    }
    renderFeature(features[entry.featureIndex], proj, backend, result);
  }

  return result;
}

}// namespace chart_view::runtime
