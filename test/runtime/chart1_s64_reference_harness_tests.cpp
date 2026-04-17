#include <catch2/catch_test_macros.hpp>

#include "feature_layer_renderer.hpp"
#include "label_layout.hpp"
#include "portrayal/feature_symbolizer.hpp"
#include "projection/projection_context.hpp"
#include "projection/projected_viewport.hpp"
#include "rhi_render_backend.hpp"
#include "scene_builder_from_senc.hpp"
#include "senc/senc_reader.hpp"
#include "senc/senc_writer.hpp"
#include "text_label_renderer.hpp"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {
using chart_view::runtime::FeatureLayerRenderer;
using chart_view::runtime::FeatureRenderResult;
using chart_view::runtime::LabelItem;
using chart_view::runtime::RhiRenderBackend;
using chart_view::runtime::SceneBuilderFromSenc;
using chart_view::runtime::SurfaceColor;
using chart_view::runtime::SurfacePoint;
using chart_view::runtime::ViewportState;
using chart_view::runtime::chart_data::AreaGeometry;
using chart_view::runtime::chart_data::DatasetMeta;
using chart_view::runtime::chart_data::Extent;
using chart_view::runtime::chart_data::Feature;
using chart_view::runtime::chart_data::FeatureChartDataset;
using chart_view::runtime::chart_data::LineGeometry;
using chart_view::runtime::chart_data::PointGeometry;
using chart_view::runtime::label::LabelBounds;
using chart_view::runtime::portrayal::FeatureSymbolization;
using chart_view::runtime::portrayal::FeatureSymbolizer;
using chart_view::runtime::portrayal::S52ColorScheme;
using chart_view::runtime::portrayal::S52DisplayCategory;
using chart_view::runtime::portrayal::S52DisplaySettings;
using chart_view::runtime::portrayal::S52PointSymbolMode;
using chart_view::runtime::projection::ProjectedViewport;
using chart_view::runtime::projection::ProjectionContext;

struct CropSpec
{
  std::string name;
  std::uint64_t featureId{0};
  int width{0};
  int height{0};
  int offsetX{0};
  int offsetY{0};
};

struct FeatureObservation
{
  std::uint64_t featureId{0};
  std::string objectAcronym;
  std::string ruleId;
  std::string sourceRcid;
  std::string tableName;
  std::string styleKey;
  std::string textAttributeKey;
  std::string primaryAssetId;
  std::vector<std::string> conditionIds;
  bool suppressed{false};
};

struct CropObservation
{
  std::string name;
  int x{0};
  int y{0};
  int width{0};
  int height{0};
  std::string hash;
  int nonBackgroundPixels{0};
};

struct SceneObservation
{
  std::string sceneId;
  std::string description;
  std::string colorScheme;
  std::string displayCategory;
  std::string pointSymbolMode;
  bool showSoundings{true};
  bool showTextLabels{true};
  std::vector<FeatureObservation> features;
  std::vector<CropObservation> crops;
  int visibleLabelCount{0};
  int unicodeVisibleLabelCount{0};
};

struct RenderedScene
{
  FeatureChartDataset dataset;
  FeatureRenderResult renderResult;
  std::vector<std::uint8_t> rgba;
  ProjectedViewport projectedViewport;
  SurfaceColor backgroundColor{};
  std::unordered_map<std::uint64_t, SurfacePoint> anchors;
  std::unordered_map<std::uint64_t, LabelItem> visibleLabels;
  std::vector<FeatureObservation> features;
  int visibleLabelCount{0};
  int unicodeVisibleLabelCount{0};
};

FeatureChartDataset makeDataset(
  const char *name,
  const Extent &extent,
  std::vector<Feature> features)
{
  FeatureChartDataset dataset;
  DatasetMeta meta;
  meta.name = name;
  meta.sourceType = chart_view_chart_source_s57;
  meta.extent = extent;
  dataset.setMeta(std::move(meta));

  for(auto &feature : features) {
    dataset.addFeature(std::move(feature));
  }

  return dataset;
}

chart_view_viewport_t makeViewport(double scaleDenominator = 100000.0) noexcept
{
  chart_view_viewport_t viewport{};
  viewport.center_lon = 0.0;
  viewport.center_lat = 51.0;
  viewport.scale_denominator = scaleDenominator;
  viewport.pixel_width = 800;
  viewport.pixel_height = 600;
  return viewport;
}

FeatureChartDataset roundtripDataset(const FeatureChartDataset &source)
{
  chart_view::runtime::senc::SencWriter writer;
  const auto blob = writer.write(source);
  REQUIRE_FALSE(blob.empty());

  chart_view::runtime::senc::SencReader reader;
  const auto readback = reader.read(blob);
  REQUIRE(readback.ok);
  return readback.dataset;
}

const Feature *findFeatureById(const FeatureChartDataset &dataset, std::uint64_t featureId)
{
  const auto it = std::find_if(
    dataset.features().begin(),
    dataset.features().end(),
    [&](const auto &feature) { return feature.id == featureId; });
  return it == dataset.features().end() ? nullptr : &(*it);
}

std::string primaryAssetId(const FeatureSymbolization &symbolization)
{
  if(!symbolization.s52Lookup.has_value()) {
    return {};
  }

  for(const auto &instruction : symbolization.s52Lookup->instructions) {
    const auto assetId = chart_view::runtime::portrayal::instructionAssetId(instruction);
    if(!assetId.empty()) {
      return std::string(assetId);
    }
  }

  return {};
}

std::vector<std::string> conditionIds(const FeatureSymbolization &symbolization)
{
  std::vector<std::string> result;
  if(!symbolization.s52Lookup.has_value()) {
    return result;
  }

  for(const auto &instruction : symbolization.s52Lookup->instructions) {
    if(const auto *conditionalInstruction =
         std::get_if<chart_view::runtime::portrayal::S52ConditionalInstruction>(&instruction);
       conditionalInstruction != nullptr) {
      result.push_back(conditionalInstruction->conditionId);
    }
  }

  return result;
}

bool hasNonAsciiGlyph(const LabelItem &label)
{
  return std::any_of(
    label.glyphText.begin(),
    label.glyphText.end(),
    [](char32_t codePoint) { return codePoint > 0x7FU; });
}

bool regionHasColor(
  std::span<const std::uint8_t> rgba,
  int width,
  int height,
  const LabelBounds &bounds,
  const SurfaceColor &color)
{
  for(int y = (std::max)(0, bounds.top); y <= (std::min)(height - 1, bounds.bottom); ++y) {
    for(int x = (std::max)(0, bounds.left); x <= (std::min)(width - 1, bounds.right); ++x) {
      const auto offset = static_cast<std::size_t>((y * width + x) * 4);
      if(offset + 3 >= rgba.size()) {
        continue;
      }
      if(rgba[offset + 0] == color[0] && rgba[offset + 1] == color[1] && rgba[offset + 2] == color[2]
         && rgba[offset + 3] == color[3]) {
        return true;
      }
    }
  }

  return false;
}

std::string fnv1a64Hex(const std::vector<std::uint8_t> &bytes)
{
  constexpr std::uint64_t kOffset = 1469598103934665603ULL;
  constexpr std::uint64_t kPrime = 1099511628211ULL;

  std::uint64_t hash = kOffset;
  for(const auto byte : bytes) {
    hash ^= static_cast<std::uint64_t>(byte);
    hash *= kPrime;
  }

  return QStringLiteral("%1").arg(static_cast<qulonglong>(hash), 16, 16, QLatin1Char('0')).toStdString();
}

std::string toString(S52ColorScheme value)
{
  switch(value) {
  case S52ColorScheme::kDay:
    return "day";
  case S52ColorScheme::kDusk:
    return "dusk";
  case S52ColorScheme::kNight:
    return "night";
  }
  return "unknown";
}

std::string toString(S52DisplayCategory value)
{
  switch(value) {
  case S52DisplayCategory::kDisplayBase:
    return "display_base";
  case S52DisplayCategory::kStandard:
    return "standard";
  case S52DisplayCategory::kAll:
    return "all";
  }
  return "unknown";
}

std::string toString(S52PointSymbolMode value)
{
  switch(value) {
  case S52PointSymbolMode::kTraditional:
    return "traditional";
  case S52PointSymbolMode::kSimplified:
    return "simplified";
  }
  return "unknown";
}

QJsonObject toJson(const FeatureObservation &observation)
{
  QJsonArray conditionIds;
  for(const auto &conditionId : observation.conditionIds) {
    conditionIds.push_back(QString::fromStdString(conditionId));
  }

  return {
    {"featureId", QString::number(static_cast<qulonglong>(observation.featureId))},
    {"objectAcronym", QString::fromStdString(observation.objectAcronym)},
    {"ruleId", QString::fromStdString(observation.ruleId)},
    {"sourceRcid", QString::fromStdString(observation.sourceRcid)},
    {"tableName", QString::fromStdString(observation.tableName)},
    {"styleKey", QString::fromStdString(observation.styleKey)},
    {"textAttributeKey", QString::fromStdString(observation.textAttributeKey)},
    {"primaryAssetId", QString::fromStdString(observation.primaryAssetId)},
    {"conditionIds", conditionIds},
    {"suppressed", observation.suppressed}};
}

QJsonObject toJson(const CropObservation &observation)
{
  return {
    {"name", QString::fromStdString(observation.name)},
    {"x", observation.x},
    {"y", observation.y},
    {"width", observation.width},
    {"height", observation.height},
    {"hash", QString::fromStdString(observation.hash)},
    {"nonBackgroundPixels", observation.nonBackgroundPixels}};
}

QJsonObject toJson(const SceneObservation &observation)
{
  QJsonArray features;
  for(const auto &feature : observation.features) {
    features.push_back(toJson(feature));
  }

  QJsonArray crops;
  for(const auto &crop : observation.crops) {
    crops.push_back(toJson(crop));
  }

  return {
    {"sceneId", QString::fromStdString(observation.sceneId)},
    {"description", QString::fromStdString(observation.description)},
    {"colorScheme", QString::fromStdString(observation.colorScheme)},
    {"displayCategory", QString::fromStdString(observation.displayCategory)},
    {"pointSymbolMode", QString::fromStdString(observation.pointSymbolMode)},
    {"showSoundings", observation.showSoundings},
    {"showTextLabels", observation.showTextLabels},
    {"visibleLabelCount", observation.visibleLabelCount},
    {"unicodeVisibleLabelCount", observation.unicodeVisibleLabelCount},
    {"features", features},
    {"crops", crops}};
}

FeatureObservation parseFeatureObservation(const QJsonObject &object)
{
  std::vector<std::string> conditionIds;
  for(const auto &value : object.value("conditionIds").toArray()) {
    conditionIds.push_back(value.toString().toStdString());
  }

  return {
    object.value("featureId").toString().toULongLong(),
    object.value("objectAcronym").toString().toStdString(),
    object.value("ruleId").toString().toStdString(),
    object.value("sourceRcid").toString().toStdString(),
    object.value("tableName").toString().toStdString(),
    object.value("styleKey").toString().toStdString(),
    object.value("textAttributeKey").toString().toStdString(),
    object.value("primaryAssetId").toString().toStdString(),
    std::move(conditionIds),
    object.value("suppressed").toBool()};
}

CropObservation parseCropObservation(const QJsonObject &object)
{
  return {
    object.value("name").toString().toStdString(),
    object.value("x").toInt(),
    object.value("y").toInt(),
    object.value("width").toInt(),
    object.value("height").toInt(),
    object.value("hash").toString().toStdString(),
    object.value("nonBackgroundPixels").toInt()};
}

SceneObservation parseSceneObservation(const QJsonObject &object)
{
  SceneObservation scene;
  scene.sceneId = object.value("sceneId").toString().toStdString();
  scene.description = object.value("description").toString().toStdString();
  scene.colorScheme = object.value("colorScheme").toString().toStdString();
  scene.displayCategory = object.value("displayCategory").toString().toStdString();
  scene.pointSymbolMode = object.value("pointSymbolMode").toString().toStdString();
  scene.showSoundings = object.value("showSoundings").toBool();
  scene.showTextLabels = object.value("showTextLabels").toBool();
  scene.visibleLabelCount = object.value("visibleLabelCount").toInt();
  scene.unicodeVisibleLabelCount = object.value("unicodeVisibleLabelCount").toInt();

  for(const auto &feature : object.value("features").toArray()) {
    scene.features.push_back(parseFeatureObservation(feature.toObject()));
  }
  for(const auto &crop : object.value("crops").toArray()) {
    scene.crops.push_back(parseCropObservation(crop.toObject()));
  }
  return scene;
}

QString referencePath(std::string_view sceneId)
{
  return QDir(QStringLiteral(CHART_VIEW_PROJECT_SOURCE_DIR))
    .filePath(QStringLiteral("tests/data/reference/%1.reference.json").arg(QString::fromStdString(std::string(sceneId))));
}

void maybeWriteReference(const SceneObservation &observation)
{
  if(!qEnvironmentVariableIsSet("CHART_VIEW_WRITE_REFERENCE")) {
    return;
  }

  const QString path = referencePath(observation.sceneId);
  const QFileInfo info(path);
  QDir().mkpath(info.dir().absolutePath());

  QFile file(path);
  REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
  file.write(QJsonDocument(toJson(observation)).toJson(QJsonDocument::Indented));
  file.close();
}

SceneObservation loadReference(std::string_view sceneId)
{
  QFile file(referencePath(sceneId));
  REQUIRE(file.exists());
  REQUIRE(file.open(QIODevice::ReadOnly));
  const auto document = QJsonDocument::fromJson(file.readAll());
  REQUIRE(document.isObject());
  return parseSceneObservation(document.object());
}

CropObservation captureCrop(
  std::string name,
  std::span<const std::uint8_t> rgba,
  int frameWidth,
  int frameHeight,
  const SurfaceColor &background,
  int x,
  int y,
  int width,
  int height)
{
  CropObservation crop;
  crop.name = std::move(name);
  crop.x = x;
  crop.y = y;
  crop.width = width;
  crop.height = height;

  std::vector<std::uint8_t> bytes;
  bytes.reserve(static_cast<std::size_t>(width * height * 4));
  for(int row = 0; row < height; ++row) {
    for(int column = 0; column < width; ++column) {
      const auto sampleX = (std::clamp)(x + column, 0, frameWidth - 1);
      const auto sampleY = (std::clamp)(y + row, 0, frameHeight - 1);
      const auto offset = static_cast<std::size_t>((sampleY * frameWidth + sampleX) * 4);
      const auto r = rgba[offset + 0];
      const auto g = rgba[offset + 1];
      const auto b = rgba[offset + 2];
      const auto a = rgba[offset + 3];
      bytes.push_back(r);
      bytes.push_back(g);
      bytes.push_back(b);
      bytes.push_back(a);
      if(r != background[0] || g != background[1] || b != background[2] || a != background[3]) {
        ++crop.nonBackgroundPixels;
      }
    }
  }

  crop.hash = fnv1a64Hex(bytes);
  return crop;
}

RenderedScene renderScene(
  const FeatureChartDataset &source,
  const S52DisplaySettings &settings)
{
  const auto dataset = roundtripDataset(source);
  const auto viewport = makeViewport();

  ViewportState viewportState;
  viewportState.set(viewport);

  SceneBuilderFromSenc builder;
  const auto snapshot = builder.buildAll(dataset, viewportState);
  REQUIRE(snapshot != nullptr);

  const auto projectionContext = ProjectionContext::createMercator();
  REQUIRE(projectionContext.isValid());

  ProjectedViewport projectedViewport;
  REQUIRE(ProjectedViewport::create(viewport, projectionContext, projectedViewport));

  RhiRenderBackend backend;
  REQUIRE(backend.initialize(viewport.pixel_width, viewport.pixel_height) == chart_view_status_ok);

  FeatureLayerRenderer renderer(settings);
  const auto renderResult = renderer.render(*snapshot, dataset, backend);
  REQUIRE(renderResult.status == chart_view_status_ok);

  std::vector<std::uint8_t> rgba(backend.frameByteSize(), 0U);
  REQUIRE(backend.copyFrameRgba(std::span<std::uint8_t>(rgba)) == chart_view_status_ok);

  FeatureSymbolizer symbolizer(settings);
  chart_view::runtime::TextLabelRenderer textRenderer;

  RenderedScene rendered{
    dataset,
    renderResult,
    std::move(rgba),
    projectedViewport,
    renderer.portrayalRegistry().canvasBackgroundColor()};

  for(const auto &feature : dataset.features()) {
    const auto symbolization = symbolizer.symbolize(feature);
    rendered.features.push_back(FeatureObservation{
      feature.id,
      feature.classAcronym,
      symbolization.s52Lookup.has_value() ? symbolization.s52Lookup->ruleId : std::string{},
      symbolization.s52Lookup.has_value() ? symbolization.s52Lookup->sourceRcid : std::string{},
      symbolization.s52Lookup.has_value() ? symbolization.s52Lookup->tableName : std::string{},
      symbolization.styleKey,
      symbolization.textAttributeKey,
      primaryAssetId(symbolization),
      conditionIds(symbolization),
      symbolization.suppressed});

    SurfacePoint anchor{};
    REQUIRE(chart_view::runtime::label::resolveProjectedLabelAnchor(
      feature,
      projectionContext,
      projectedViewport,
      anchor));
    rendered.anchors.emplace(feature.id, anchor);

    if(symbolization.suppressed || symbolization.textKey.empty()) {
      continue;
    }

    const auto textRule = renderer.portrayalRegistry().resolveTextRuleForStyle(symbolization.textKey);
    const auto label = textRenderer.layout(
      symbolization.textKey,
      feature,
      anchor,
      textRule,
      symbolization.textAttributeKey);
    if(!label.has_value()) {
      continue;
    }

    if(chart_view::runtime::label::labelBoundsVisible(
         label->bounds,
         projectedViewport.pixelWidth,
         projectedViewport.pixelHeight)
       && regionHasColor(
         rendered.rgba,
         projectedViewport.pixelWidth,
         projectedViewport.pixelHeight,
         label->bounds,
         textRule.color)) {
      rendered.visibleLabels.emplace(feature.id, *label);
      ++rendered.visibleLabelCount;
      if(hasNonAsciiGlyph(*label)) {
        ++rendered.unicodeVisibleLabelCount;
      }
    }
  }

  std::sort(
    rendered.features.begin(),
    rendered.features.end(),
    [](const auto &lhs, const auto &rhs) { return lhs.featureId < rhs.featureId; });

  return rendered;
}

SceneObservation observeScene(
  std::string sceneId,
  std::string description,
  const S52DisplaySettings &settings,
  const RenderedScene &rendered,
  std::span<const CropSpec> cropSpecs)
{
  SceneObservation observation;
  observation.sceneId = std::move(sceneId);
  observation.description = std::move(description);
  observation.colorScheme = toString(settings.colorScheme);
  observation.displayCategory = toString(settings.displayCategory);
  observation.pointSymbolMode = toString(settings.pointSymbolMode);
  observation.showSoundings = settings.showSoundings;
  observation.showTextLabels = settings.showTextLabels;
  observation.features = rendered.features;
  observation.visibleLabelCount = rendered.visibleLabelCount;
  observation.unicodeVisibleLabelCount = rendered.unicodeVisibleLabelCount;

  for(const auto &cropSpec : cropSpecs) {
    const auto anchorIt = rendered.anchors.find(cropSpec.featureId);
    REQUIRE(anchorIt != rendered.anchors.end());
    observation.crops.push_back(captureCrop(
      cropSpec.name,
      rendered.rgba,
      rendered.projectedViewport.pixelWidth,
      rendered.projectedViewport.pixelHeight,
      rendered.backgroundColor,
      anchorIt->second.x - cropSpec.width / 2 + cropSpec.offsetX,
      anchorIt->second.y - cropSpec.height / 2 + cropSpec.offsetY,
      cropSpec.width,
      cropSpec.height));
  }

  std::sort(
    observation.crops.begin(),
    observation.crops.end(),
    [](const auto &lhs, const auto &rhs) { return lhs.name < rhs.name; });

  return observation;
}

void verifyObservation(const SceneObservation &expected, const SceneObservation &observed)
{
  REQUIRE(expected.sceneId == observed.sceneId);
  REQUIRE(expected.description == observed.description);
  REQUIRE(expected.colorScheme == observed.colorScheme);
  REQUIRE(expected.displayCategory == observed.displayCategory);
  REQUIRE(expected.pointSymbolMode == observed.pointSymbolMode);
  REQUIRE(expected.showSoundings == observed.showSoundings);
  REQUIRE(expected.showTextLabels == observed.showTextLabels);
  REQUIRE(expected.visibleLabelCount == observed.visibleLabelCount);
  REQUIRE(expected.unicodeVisibleLabelCount == observed.unicodeVisibleLabelCount);
  REQUIRE(expected.features.size() == observed.features.size());
  REQUIRE(expected.crops.size() == observed.crops.size());

  for(std::size_t index = 0; index < expected.features.size(); ++index) {
    const auto &lhs = expected.features[index];
    const auto &rhs = observed.features[index];
    REQUIRE(lhs.featureId == rhs.featureId);
    REQUIRE(lhs.objectAcronym == rhs.objectAcronym);
    REQUIRE(lhs.ruleId == rhs.ruleId);
    REQUIRE(lhs.sourceRcid == rhs.sourceRcid);
    REQUIRE(lhs.tableName == rhs.tableName);
    REQUIRE(lhs.styleKey == rhs.styleKey);
    REQUIRE(lhs.textAttributeKey == rhs.textAttributeKey);
    REQUIRE(lhs.primaryAssetId == rhs.primaryAssetId);
    REQUIRE(lhs.conditionIds == rhs.conditionIds);
    REQUIRE(lhs.suppressed == rhs.suppressed);
  }

  for(std::size_t index = 0; index < expected.crops.size(); ++index) {
    const auto &lhs = expected.crops[index];
    const auto &rhs = observed.crops[index];
    REQUIRE(lhs.name == rhs.name);
    REQUIRE(lhs.x == rhs.x);
    REQUIRE(lhs.y == rhs.y);
    REQUIRE(lhs.width == rhs.width);
    REQUIRE(lhs.height == rhs.height);
    REQUIRE(lhs.hash == rhs.hash);
    REQUIRE(lhs.nonBackgroundPixels == rhs.nonBackgroundPixels);
  }
}

FeatureChartDataset makeChart1Scene()
{
  Feature landArea;
  landArea.id = 101;
  landArea.classCode = 71;
  landArea.classAcronym = "LNDARE";
  landArea.geometry = AreaGeometry{{{-0.16, 50.90}, {0.16, 50.90}, {0.16, 51.10}, {-0.16, 51.10}}, {}};

  Feature fairway;
  fairway.id = 102;
  fairway.classCode = 59;
  fairway.classAcronym = "FAIRWY";
  fairway.geometry = LineGeometry{{{-0.14, 50.985}, {0.14, 50.985}}};

  Feature buoy;
  buoy.id = 103;
  buoy.classCode = 19;
  buoy.classAcronym = "BOYSPP";
  buoy.geometry = PointGeometry{{0.0, 51.0}};
  buoy.attributes["OBJNAM"] = std::string("Reference Buoy");

  Feature wreck;
  wreck.id = 104;
  wreck.classCode = 159;
  wreck.classAcronym = "WRECKS";
  wreck.geometry = PointGeometry{{0.08, 51.03}};
  wreck.attributes["OBJNAM"] = std::string("Harbor Wreck");
  wreck.attributes["NOBJNM"] = std::string(reinterpret_cast<const char *>(u8"\u6e2f\u53e3\u6c89\u8239"));

  return makeDataset(
    "phase6a_chart1_day_standard",
    {-0.3, 50.85, 0.3, 51.15},
    {landArea, fairway, buoy, wreck});
}

FeatureChartDataset makeS64Scene()
{
  Feature buoy;
  buoy.id = 201;
  buoy.classCode = 19;
  buoy.classAcronym = "BOYSPP";
  buoy.geometry = PointGeometry{{0.0, 51.0}};
  buoy.attributes["OBJNAM"] = std::string("Reference Buoy");

  Feature sounding;
  sounding.id = 202;
  sounding.classCode = 129;
  sounding.classAcronym = "SOUNDG";
  sounding.geometry = PointGeometry{{-0.06, 51.0}};
  sounding.attributes["VALSOU"] = 9.1;

  Feature wreck;
  wreck.id = 203;
  wreck.classCode = 159;
  wreck.classAcronym = "WRECKS";
  wreck.geometry = PointGeometry{{0.07, 51.02}};
  wreck.attributes["OBJNAM"] = std::string("Reference Wreck");
  wreck.attributes["NOBJNM"] = std::string(reinterpret_cast<const char *>(u8"\u6d4b\u8bd5\u6c89\u8239"));

  return makeDataset(
    "phase6a_s64_reference",
    {-0.3, 50.85, 0.3, 51.15},
    {buoy, sounding, wreck});
}

void runReferenceHarness(
  const std::string &sceneId,
  const std::string &description,
  const FeatureChartDataset &dataset,
  const S52DisplaySettings &settings,
  std::span<const CropSpec> cropSpecs)
{
  const auto rendered = renderScene(dataset, settings);
  const auto observed = observeScene(sceneId, description, settings, rendered, cropSpecs);
  maybeWriteReference(observed);
  const auto expected = loadReference(sceneId);
  verifyObservation(expected, observed);
}

}// namespace

TEST_CASE("Phase 6A graphical harness matches the committed Chart 1 reference scene", "[phase6a][chart1][reference][rhi]")
{
  const std::array<CropSpec, 3> crops{{
    {"buoy_symbol", 103, 24, 24, 0, 0},
    {"fairway_center", 102, 40, 18, 0, 0},
    {"land_fill", 101, 28, 28, 0, 0},
  }};

  S52DisplaySettings settings;
  settings.colorScheme = S52ColorScheme::kDay;
  settings.displayCategory = S52DisplayCategory::kStandard;
  settings.pointSymbolMode = S52PointSymbolMode::kTraditional;
  runReferenceHarness(
    "phase6a_chart1_day_standard",
    "Chart 1-inspired day/standard harbor scene",
    makeChart1Scene(),
    settings,
    crops);
}

TEST_CASE("Phase 6A graphical harness matches the committed S-64 traditional reference scene", "[phase6a][s64][reference][rhi]")
{
  const std::array<CropSpec, 3> crops{{
    {"buoy_symbol", 201, 24, 24, 0, 0},
    {"sounding_symbol", 202, 24, 24, 0, 0},
    {"danger_symbol", 203, 24, 24, 0, 0},
  }};

  S52DisplaySettings settings;
  settings.colorScheme = S52ColorScheme::kDay;
  settings.displayCategory = S52DisplayCategory::kStandard;
  settings.pointSymbolMode = S52PointSymbolMode::kTraditional;
  settings.showSoundings = true;
  settings.showTextLabels = true;
  runReferenceHarness(
    "phase6a_s64_traditional",
    "S-64-inspired traditional point and label scene",
    makeS64Scene(),
    settings,
    crops);
}

TEST_CASE("Phase 6A graphical harness matches the committed S-64 simplified suppression reference scene", "[phase6a][s64][reference][rhi]")
{
  const std::array<CropSpec, 3> crops{{
    {"buoy_symbol", 201, 24, 24, 0, 0},
    {"sounding_background", 202, 24, 24, 0, 0},
    {"danger_symbol", 203, 24, 24, 0, 0},
  }};

  S52DisplaySettings settings;
  settings.colorScheme = S52ColorScheme::kDay;
  settings.displayCategory = S52DisplayCategory::kStandard;
  settings.pointSymbolMode = S52PointSymbolMode::kSimplified;
  settings.showSoundings = false;
  settings.showTextLabels = false;
  runReferenceHarness(
    "phase6a_s64_simplified",
    "S-64-inspired simplified symbols with suppressed soundings and labels",
    makeS64Scene(),
    settings,
    crops);
}
