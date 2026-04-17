#include "feature_layer_renderer.hpp"

#include "portrayal/display_priority_model.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <type_traits>
#include <variant>

namespace chart_view::runtime {

namespace {

constexpr double kProjectionGuardFactor = 8.0;

bool isFiniteCoordinate(const chart_data::Coordinate &coord) noexcept
{
  return std::isfinite(coord.lon) && std::isfinite(coord.lat);
}

bool tryProjectLegacyToPixel(
  const FeatureLayerRenderer::ViewportProjection &proj,
  const chart_data::Coordinate &coord,
  SurfacePoint &outPoint)
{
  if(!isFiniteCoordinate(coord)) {
    return false;
  }

  const double ndcX = (coord.lon - proj.centerLon) * proj.scaleX;
  const double ndcY = (coord.lat - proj.centerLat) * proj.scaleY;
  if(!std::isfinite(ndcX) || !std::isfinite(ndcY)) {
    return false;
  }

  const double pixelX = (ndcX + 1.0) * 0.5 * static_cast<double>(proj.pixelWidth - 1);
  const double pixelY = (1.0 - (ndcY + 1.0) * 0.5) * static_cast<double>(proj.pixelHeight - 1);
  if(!std::isfinite(pixelX) || !std::isfinite(pixelY)) {
    return false;
  }

  const double guardX = std::max(1.0, static_cast<double>(proj.pixelWidth) * kProjectionGuardFactor);
  const double guardY = std::max(1.0, static_cast<double>(proj.pixelHeight) * kProjectionGuardFactor);
  if(pixelX < -guardX || pixelX > static_cast<double>(proj.pixelWidth - 1) + guardX ||
     pixelY < -guardY || pixelY > static_cast<double>(proj.pixelHeight - 1) + guardY) {
    return false;
  }

  outPoint = {
    static_cast<int>(std::lround(pixelX)),
    static_cast<int>(std::lround(pixelY))};
  return true;
}

std::optional<SurfacePoint> resolveLegacyLabelAnchor(
  const chart_data::Feature &feature,
  const FeatureLayerRenderer::ViewportProjection &proj)
{
  std::optional<SurfacePoint> anchor;
  std::visit(
    [&](auto &&geom) {
      using T = std::decay_t<decltype(geom)>;

      if constexpr(std::is_same_v<T, chart_data::PointGeometry>) {
        SurfacePoint projected{};
        if(tryProjectLegacyToPixel(proj, geom.position, projected)) {
          anchor = projected;
        }
      } else if constexpr(std::is_same_v<T, chart_data::LineGeometry>) {
        if(geom.vertices.empty()) {
          return;
        }

        const auto midpointIndex = geom.vertices.size() / 2U;
        SurfacePoint projected{};
        if(tryProjectLegacyToPixel(proj, geom.vertices[midpointIndex], projected)) {
          anchor = projected;
        }
      } else if constexpr(std::is_same_v<T, chart_data::AreaGeometry>) {
        if(geom.exteriorRing.empty()) {
          return;
        }

        double lonSum = 0.0;
        double latSum = 0.0;
        std::size_t count = 0;
        for(const auto &vertex : geom.exteriorRing) {
          if(!isFiniteCoordinate(vertex)) {
            continue;
          }
          lonSum += vertex.lon;
          latSum += vertex.lat;
          ++count;
        }
        if(count == 0) {
          return;
        }

        SurfacePoint projected{};
        if(tryProjectLegacyToPixel(
             proj,
             {lonSum / static_cast<double>(count), latSum / static_cast<double>(count)},
             projected)) {
          anchor = projected;
        }
      }
    },
    feature.geometry);

  return anchor;
}

const portrayal::S52Instruction *findInstruction(
  const portrayal::FeatureSymbolization &symbolization,
  portrayal::S52InstructionType type) noexcept
{
  if(!symbolization.s52Lookup.has_value()) {
    return nullptr;
  }

  const auto &instructions = symbolization.s52Lookup->instructions;
  const auto it = std::find_if(
    instructions.begin(),
    instructions.end(),
    [type](const portrayal::S52Instruction &instruction) {
      return portrayal::instructionType(instruction) == type;
    });
  return it == instructions.end() ? nullptr : &(*it);
}

std::string_view resolveInstructionStyleKey(
  const portrayal::FeatureSymbolization &symbolization,
  portrayal::S52InstructionType type,
  std::string_view fallback) noexcept
{
  if(const auto *instruction = findInstruction(symbolization, type); instruction != nullptr
     && !portrayal::instructionStyleKey(*instruction).empty()) {
    return portrayal::instructionStyleKey(*instruction);
  }

  return fallback;
}

std::string_view resolveInstructionAssetId(
  const portrayal::FeatureSymbolization &symbolization,
  portrayal::S52InstructionType type) noexcept
{
  if(const auto *instruction = findInstruction(symbolization, type); instruction != nullptr) {
    return portrayal::instructionAssetId(*instruction);
  }

  return {};
}

bool hasConditionalInstruction(
  const portrayal::FeatureSymbolization &symbolization,
  portrayal::S52ConditionalOpcode opcode) noexcept
{
  if(!symbolization.s52Lookup.has_value()) {
    return false;
  }

  return std::any_of(
    symbolization.s52Lookup->instructions.begin(),
    symbolization.s52Lookup->instructions.end(),
    [&](const portrayal::S52Instruction &instruction) {
      const auto *conditional = std::get_if<portrayal::S52ConditionalInstruction>(&instruction);
      return conditional != nullptr && conditional->opcode == opcode;
    });
}

std::string_view resolveConditionalStyleKey(
  const portrayal::FeatureSymbolization &symbolization,
  std::string_view baseStyleKey) noexcept
{
  if(baseStyleKey == "point/landmark"
     && hasConditionalInstruction(symbolization, portrayal::S52ConditionalOpcode::kFullSectorLights)) {
    return "point/light_sector";
  }

  if(baseStyleKey == "area/depth") {
    if(hasConditionalInstruction(symbolization, portrayal::S52ConditionalOpcode::kShallowPattern)) {
      return "area/depth_shallow_pattern";
    }
    if(hasConditionalInstruction(symbolization, portrayal::S52ConditionalOpcode::kSafetyContourAlert)) {
      return "area/depth_safety_alert";
    }
    if(hasConditionalInstruction(symbolization, portrayal::S52ConditionalOpcode::kTwoShadesDepth)) {
      if(hasConditionalInstruction(symbolization, portrayal::S52ConditionalOpcode::kSymbolizedBoundaries)) {
        return "area/depth_two_shades_symbolized_boundary";
      }
      if(hasConditionalInstruction(symbolization, portrayal::S52ConditionalOpcode::kPlainBoundaries)) {
        return "area/depth_two_shades_plain_boundary";
      }
      return "area/depth_two_shades";
    }
    if(hasConditionalInstruction(symbolization, portrayal::S52ConditionalOpcode::kFullDepthShades)) {
      if(hasConditionalInstruction(symbolization, portrayal::S52ConditionalOpcode::kSymbolizedBoundaries)) {
        return "area/depth_full_shades_symbolized_boundary";
      }
      if(hasConditionalInstruction(symbolization, portrayal::S52ConditionalOpcode::kPlainBoundaries)) {
        return "area/depth_full_shades_plain_boundary";
      }
      return "area/depth_full_shades";
    }
    if(hasConditionalInstruction(symbolization, portrayal::S52ConditionalOpcode::kSymbolizedBoundaries)) {
      return "area/depth_symbolized_boundary";
    }
    if(hasConditionalInstruction(symbolization, portrayal::S52ConditionalOpcode::kPlainBoundaries)) {
      return "area/depth_plain_boundary";
    }
  }

  return baseStyleKey;
}
}// namespace

FeatureLayerRenderer::FeatureLayerRenderer()
  : FeatureLayerRenderer(portrayal::S52DisplaySettings{})
{
}

FeatureLayerRenderer::FeatureLayerRenderer(portrayal::S52DisplaySettings settings)
  : m_portrayal(settings)
  , m_symbolizer(settings)
  , m_areaSymbols()
  , m_lineSymbols()
  , m_pointSymbols()
  , m_textLabels()
{
}

void FeatureLayerRenderer::setS52Settings(const portrayal::S52DisplaySettings &settings) noexcept
{
  const auto paletteChanged = m_symbolizer.s52Settings().colorScheme != settings.colorScheme;
  m_symbolizer.setS52Settings(settings);
  if(paletteChanged) {
    m_portrayal = portrayal::PortrayalRegistry(settings);
  }
}

const portrayal::S52DisplaySettings &FeatureLayerRenderer::s52Settings() const noexcept
{
  return m_symbolizer.s52Settings();
}

void FeatureLayerRenderer::setS57ClassFilters(std::span<const portrayal::S57ClassSelectionFilter> filters)
{
  m_symbolizer.setS57ClassFilters(filters);
}

void FeatureLayerRenderer::setS52RuleFilters(std::span<const portrayal::S52RuleSelectionFilter> filters)
{
  m_symbolizer.setS52RuleFilters(filters);
}

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
  const portrayal::FeatureSymbolization &symbolization,
  const ViewportProjection &proj,
  const projection::ProjectionContext *geometryProjectionContext,
  const projection::ProjectedViewport *geometryViewport,
  RhiRenderBackend &backend,
  FeatureRenderResult &result) const
{
  if(symbolization.suppressed) {
    return;
  }

  std::visit(
    [&](auto &&geom) {
      using T = std::decay_t<decltype(geom)>;

      if constexpr(std::is_same_v<T, chart_data::PointGeometry>) {
        if(symbolization.styleKey.empty()) {
          return;
        }

        SurfacePoint point;
        const auto projectedPointResolved =
          geometryProjectionContext != nullptr
          && geometryViewport != nullptr
          && label::resolveProjectedLabelAnchor(
            feature,
            *geometryProjectionContext,
            *geometryViewport,
            point);
        if(!projectedPointResolved && !tryProjectLegacyToPixel(proj, geom.position, point)) {
          return;
        }

        const auto pointStyleKey = resolveInstructionStyleKey(
          symbolization,
          portrayal::S52InstructionType::kPointSymbol,
          symbolization.styleKey);
        const auto resolvedPointStyleKey = resolveConditionalStyleKey(symbolization, pointStyleKey);
        const auto pointAssetId = resolveInstructionAssetId(
          symbolization,
          portrayal::S52InstructionType::kPointSymbol);
        const auto &rule = m_portrayal.resolveSymbolRuleForStyle(resolvedPointStyleKey);
        if(!m_pointSymbols.render(pointAssetId, resolvedPointStyleKey, point, rule, backend)) {
          backend.drawPoint(point, rule.radius, rule.color);
        }
        ++result.pointsRendered;
        ++result.totalVertices;

      } else if constexpr(std::is_same_v<T, chart_data::LineGeometry>) {
        if(symbolization.styleKey.empty()) {
          return;
        }

        if(geom.vertices.size() < 2) {
          return;
        }

        const auto lineStyleKey = resolveInstructionStyleKey(
          symbolization,
          portrayal::S52InstructionType::kLineStyle,
          symbolization.styleKey);
        const auto lineAssetId = resolveInstructionAssetId(
          symbolization,
          portrayal::S52InstructionType::kLineStyle);
        const auto &rule = m_portrayal.resolveLineStyleRuleForStyle(lineStyleKey);
        std::vector<SurfacePoint> points;
        points.reserve(geom.vertices.size());
        std::uint32_t vertexCount = 0;
        for(const auto &vertex : geom.vertices) {
          SurfacePoint point;
          if(!tryProjectLegacyToPixel(proj, vertex, point)) {
            return;
          }
          points.push_back(point);
          ++vertexCount;
        }
        if(!m_lineSymbols.render(lineAssetId, lineStyleKey, points, rule, backend)) {
          for(std::size_t i = 1; i < points.size(); ++i) {
            backend.drawLine(points[i - 1], points[i], rule.thickness, rule.color);
          }
        }
        result.totalVertices += vertexCount;
        ++result.linesRendered;

      } else if constexpr(std::is_same_v<T, chart_data::AreaGeometry>) {
        if(symbolization.styleKey.empty()) {
          return;
        }

        if(geom.exteriorRing.size() < 3) {
          return;
        }

        const auto areaStyleKey = resolveInstructionStyleKey(
          symbolization,
          portrayal::S52InstructionType::kAreaPattern,
          symbolization.styleKey);
        const auto resolvedAreaStyleKey = resolveConditionalStyleKey(symbolization, areaStyleKey);
        const auto areaAssetId = resolveInstructionAssetId(
          symbolization,
          portrayal::S52InstructionType::kAreaPattern);
        const auto &rule = m_portrayal.resolveAreaFillRuleForStyle(resolvedAreaStyleKey);
        std::vector<SurfacePoint> exterior;
        exterior.reserve(geom.exteriorRing.size());
        std::uint32_t vertexCount = 0;
        for(const auto &vertex : geom.exteriorRing) {
          SurfacePoint point;
          if(!tryProjectLegacyToPixel(proj, vertex, point)) {
            return;
          }
          exterior.push_back(point);
          ++vertexCount;
        }
        std::vector<std::vector<SurfacePoint>> holePointsCollection;
        holePointsCollection.reserve(geom.interiorRings.size());
        for(const auto &hole : geom.interiorRings) {
          if(hole.size() < 3) {
            continue;
          }

          std::vector<SurfacePoint> holePoints;
          holePoints.reserve(hole.size());
          for(const auto &vertex : hole) {
            SurfacePoint point;
            if(!tryProjectLegacyToPixel(proj, vertex, point)) {
              return;
            }
            holePoints.push_back(point);
          }
          holePointsCollection.push_back(std::move(holePoints));
        }

        if(!m_areaSymbols.render(areaAssetId, resolvedAreaStyleKey, exterior, holePointsCollection, rule, backend)) {
          backend.fillPolygon(exterior, rule.fillColor);
          backend.drawClosedPolyline(exterior, rule.outlineThickness, rule.outlineColor);

          for(const auto &holePoints : holePointsCollection) {
            backend.fillPolygon(holePoints, rule.holeFillColor);
            backend.drawClosedPolyline(holePoints, rule.outlineThickness, rule.outlineColor);
          }
        }
        result.totalVertices += vertexCount;
        ++result.areasRendered;
      }
    },
    feature.geometry);
}

void FeatureLayerRenderer::renderFeatureLabel(
  const chart_data::Feature &feature,
  const portrayal::FeatureSymbolization &symbolization,
  const ViewportProjection &proj,
  const projection::ProjectionContext *labelProjectionContext,
  const projection::ProjectedViewport *labelViewport,
  std::vector<label::LabelBounds> &occupiedLabelBounds,
  RhiRenderBackend &backend) const
{
  if(symbolization.suppressed) {
    return;
  }

  const auto textStyleKey = resolveInstructionStyleKey(
    symbolization,
    portrayal::S52InstructionType::kTextLabel,
    symbolization.textKey);
  if(textStyleKey.empty()) {
    return;
  }

  std::optional<SurfacePoint> anchor;
  if(labelProjectionContext != nullptr && labelViewport != nullptr) {
    SurfacePoint projectedAnchor{};
    if(label::resolveProjectedLabelAnchor(
         feature,
         *labelProjectionContext,
         *labelViewport,
         projectedAnchor)) {
      anchor = projectedAnchor;
    }
  }

  if(!anchor.has_value()) {
    anchor = resolveLegacyLabelAnchor(feature, proj);
  }
  if(!anchor.has_value()) {
    return;
  }

  const auto &rule = m_portrayal.resolveTextRuleForStyle(textStyleKey);
  const auto layoutItem = m_textLabels.layout(
    textStyleKey,
    feature,
    *anchor,
    rule,
    symbolization.textAttributeKey);
  if(!layoutItem.has_value()) {
    return;
  }

  const auto pixelWidth = labelViewport != nullptr ? labelViewport->pixelWidth : proj.pixelWidth;
  const auto pixelHeight = labelViewport != nullptr ? labelViewport->pixelHeight : proj.pixelHeight;
  if(!label::labelBoundsVisible(layoutItem->bounds, pixelWidth, pixelHeight)) {
    return;
  }

  const auto overlapsExistingLabel = std::any_of(
    occupiedLabelBounds.begin(),
    occupiedLabelBounds.end(),
    [&](const label::LabelBounds &existingBounds) {
      return label::labelBoundsOverlap(existingBounds, layoutItem->bounds);
    });
  if(overlapsExistingLabel) {
    return;
  }

  occupiedLabelBounds.push_back(layoutItem->bounds);
  m_textLabels.render(*layoutItem, backend);
}

FeatureRenderResult FeatureLayerRenderer::render(
  const SceneSnapshot &snapshot,
  std::span<const chart_data::FeatureChartDataset> datasets,
  RhiRenderBackend &backend) const
{
  FeatureRenderResult result;
  const auto &backgroundColor = m_portrayal.canvasBackgroundColor();

  if (!backend.isInitialized()) {
    result.status = chart_view_status_not_initialized;
    return result;
  }

  if (snapshot.empty()) {
    (void)backend.renderClearFrame(
      static_cast<float>(backgroundColor[0]) / 255.0F,
      static_cast<float>(backgroundColor[1]) / 255.0F,
      static_cast<float>(backgroundColor[2]) / 255.0F,
      static_cast<float>(backgroundColor[3]) / 255.0F);
    return result;
  }

  auto clearStatus = backend.renderClearFrame(
    static_cast<float>(backgroundColor[0]) / 255.0F,
    static_cast<float>(backgroundColor[1]) / 255.0F,
    static_cast<float>(backgroundColor[2]) / 255.0F,
    static_cast<float>(backgroundColor[3]) / 255.0F);
  if(clearStatus != chart_view_status_ok) {
    result.status = clearStatus;
    return result;
  }

  const auto proj = makeProjection(snapshot.viewport());
  auto symbolizer = m_symbolizer;
  auto liveSettings = symbolizer.s52Settings();
  liveSettings.viewingScaleDenominator = snapshot.viewport().scale_denominator;
  symbolizer.setS52Settings(liveSettings);
  const auto labelProjectionContext = projection::ProjectionContext::createMercator();
  projection::ProjectedViewport labelViewport{};
  const auto haveProjectedLabelViewport =
    labelProjectionContext.isValid()
    && projection::ProjectedViewport::create(
      snapshot.viewport(),
      labelProjectionContext,
      labelViewport);
  std::vector<label::LabelBounds> occupiedLabelBounds;
  if(snapshot.charts().size() > 1U) {
    for(const auto &entry : snapshot.layers()) {
      if(entry.sourceChartIndex >= datasets.size()) {
        continue;
      }

      const auto &features = datasets[entry.sourceChartIndex].features();
      if(entry.featureIndex >= features.size()) {
        continue;
      }

      const auto &feature = features[entry.featureIndex];
      const auto symbolization = symbolizer.symbolize(feature);
      renderFeature(
        feature,
        symbolization,
        proj,
        haveProjectedLabelViewport ? &labelProjectionContext : nullptr,
        haveProjectedLabelViewport ? &labelViewport : nullptr,
        backend,
        result);
    }

    for(const auto &entry : snapshot.layers()) {
      if(entry.sourceChartIndex >= datasets.size()) {
        continue;
      }

      const auto &features = datasets[entry.sourceChartIndex].features();
      if(entry.featureIndex >= features.size()) {
        continue;
      }

      const auto &feature = features[entry.featureIndex];
      const auto symbolization = symbolizer.symbolize(feature);
      renderFeatureLabel(
        feature,
        symbolization,
        proj,
        haveProjectedLabelViewport ? &labelProjectionContext : nullptr,
        haveProjectedLabelViewport ? &labelViewport : nullptr,
        occupiedLabelBounds,
        backend);
    }

    return result;
  }

  const portrayal::DisplayPriorityModel displayPriorityModel;
  const auto renderGroup = [&](portrayal::DisplayLayerGroup group) {
    for(const auto &entry : snapshot.layers()) {
      if(entry.sourceChartIndex >= datasets.size()) {
        continue;
      }

      const auto &featureSet = datasets[entry.sourceChartIndex].features();
      if(entry.featureIndex >= featureSet.size()) {
        continue;
      }

      const auto &feature = featureSet[entry.featureIndex];
      const auto symbolization = symbolizer.symbolize(feature);
      const auto displayPriority = displayPriorityModel.resolve(feature, symbolization);
      if(displayPriority.layerGroup != group) {
        continue;
      }

      renderFeature(
        feature,
        symbolization,
        proj,
        haveProjectedLabelViewport ? &labelProjectionContext : nullptr,
        haveProjectedLabelViewport ? &labelViewport : nullptr,
        backend,
        result);
    }
  };

  renderGroup(portrayal::DisplayLayerGroup::kAreas);
  renderGroup(portrayal::DisplayLayerGroup::kLines);
  renderGroup(portrayal::DisplayLayerGroup::kPoints);

  for(const auto &entry : snapshot.layers()) {
    if(entry.sourceChartIndex >= datasets.size()) {
      continue;
    }

    const auto &featureSet = datasets[entry.sourceChartIndex].features();
    if(entry.featureIndex >= featureSet.size()) {
      continue;
    }

    const auto &feature = featureSet[entry.featureIndex];
    const auto symbolization = symbolizer.symbolize(feature);
    renderFeatureLabel(
      feature,
      symbolization,
      proj,
      haveProjectedLabelViewport ? &labelProjectionContext : nullptr,
      haveProjectedLabelViewport ? &labelViewport : nullptr,
      occupiedLabelBounds,
      backend);
  }

  return result;
}

FeatureRenderResult FeatureLayerRenderer::render(
  const SceneSnapshot &snapshot,
  const chart_data::FeatureChartDataset &dataset,
  RhiRenderBackend &backend) const
{
  return render(snapshot, std::span<const chart_data::FeatureChartDataset>(&dataset, 1), backend);
}

}// namespace chart_view::runtime
