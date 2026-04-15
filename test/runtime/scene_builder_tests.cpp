#include <catch2/catch_test_macros.hpp>

#include "scene_builder_from_senc.hpp"
#include "chart_data/feature_chart_dataset.hpp"
#include "chart_data/geometry.hpp"
#include "senc/senc_writer.hpp"
#include "senc/senc_reader.hpp"

#include <cstdint>
#include <memory>
#include <vector>

using namespace chart_view::runtime;
using namespace chart_view::runtime::chart_data;
using namespace chart_view::runtime::senc;

// Helpers ----------------------------------------------------------------

static FeatureChartDataset makeTestDataset()
{
  FeatureChartDataset ds;
  DatasetMeta meta;
  meta.name = "scene_test";
  meta.sourceType = chart_view_chart_source_s57;
  meta.extent = {-5.0, 50.0, 5.0, 55.0};
  ds.setMeta(std::move(meta));

  // Feature 0: point at (0, 52) -- inside typical viewport
  Feature f0;
  f0.id = 1;
  f0.classCode = 100;
  f0.classAcronym = "LIGHTS";
  f0.geometry = PointGeometry{{0.0, 52.0}};
  ds.addFeature(std::move(f0));

  // Feature 1: line from (-4, 51) to (-3, 53) -- west side
  Feature f1;
  f1.id = 2;
  f1.classCode = 200;
  f1.classAcronym = "COALNE";
  f1.geometry = LineGeometry{{{-4.0, 51.0}, {-3.0, 53.0}}};
  ds.addFeature(std::move(f1));

  // Feature 2: area polygon around (3, 54) -- east/north side
  Feature f2;
  f2.id = 3;
  f2.classCode = 300;
  f2.classAcronym = "DEPARE";
  f2.geometry = AreaGeometry{{{2.0, 53.0}, {4.0, 53.0}, {4.0, 55.0}, {2.0, 55.0}}, {}};
  ds.addFeature(std::move(f2));

  // Feature 3: point far away at (120, -30) -- should be culled
  Feature f3;
  f3.id = 4;
  f3.classCode = 400;
  f3.classAcronym = "BOYCAR";
  f3.geometry = PointGeometry{{120.0, -30.0}};
  ds.addFeature(std::move(f3));

  return ds;
}

static ViewportState makeViewport(double lon, double lat, double scale, int w, int h)
{
  ViewportState vs;
  chart_view_viewport_t vp{};
  vp.center_lon = lon;
  vp.center_lat = lat;
  vp.scale_denominator = scale;
  vp.pixel_width = w;
  vp.pixel_height = h;
  vs.set(vp);
  return vs;
}

// Tests ------------------------------------------------------------------

TEST_CASE("SceneBuilder empty dataset produces empty snapshot", "[scene_builder]")
{
  FeatureChartDataset empty;
  auto vs = makeViewport(0, 52, 50000, 800, 600);

  SceneBuilderFromSenc builder;
  auto snap = builder.build(empty, vs);
  REQUIRE(snap != nullptr);
  REQUIRE(snap->empty());
}

TEST_CASE("SceneBuilder invalid viewport produces empty snapshot", "[scene_builder]")
{
  auto ds = makeTestDataset();
  ViewportState invalidVp;  // default: pixel_width=0, invalid

  SceneBuilderFromSenc builder;
  auto snap = builder.build(ds, invalidVp);
  REQUIRE(snap != nullptr);
  REQUIRE(snap->empty());
}

TEST_CASE("SceneBuilder culls features outside viewport", "[scene_builder]")
{
  auto ds = makeTestDataset();
  // Viewport centered at (0, 52), scale 50000 -- should see feature 0 (point at 0,52)
  // but NOT feature 3 (point at 120,-30).
  auto vs = makeViewport(0.0, 52.0, 50000.0, 800, 600);

  SceneBuilderFromSenc builder;
  auto snap = builder.build(ds, vs);
  REQUIRE(snap != nullptr);
  REQUIRE_FALSE(snap->empty());

  // Feature 3 (far away) should be culled.
  bool foundFarAway = false;
  for (const auto &entry : snap->layers()) {
    if (entry.layerId == 400) foundFarAway = true;
  }
  REQUIRE_FALSE(foundFarAway);
}

TEST_CASE("SceneBuilder includes nearby features", "[scene_builder]")
{
  auto ds = makeTestDataset();
  // Wide viewport covering all European features
  auto vs = makeViewport(0.0, 52.5, 5000000.0, 1200, 900);

  SceneBuilderFromSenc builder;
  auto snap = builder.build(ds, vs);
  REQUIRE(snap != nullptr);

  // Should include features 0, 1, 2 (all in [-5,50]-[5,55] range)
  REQUIRE(snap->layers().size() >= 3);

  bool hasLights = false;
  bool hasCoalne = false;
  bool hasDepare = false;
  for (const auto &entry : snap->layers()) {
    if (entry.layerId == 100) hasLights = true;
    if (entry.layerId == 200) hasCoalne = true;
    if (entry.layerId == 300) hasDepare = true;
  }
  REQUIRE(hasLights);
  REQUIRE(hasCoalne);
  REQUIRE(hasDepare);
}

TEST_CASE("SceneBuilder buildAll includes every feature", "[scene_builder]")
{
  auto ds = makeTestDataset();
  auto vs = makeViewport(0.0, 52.0, 50000.0, 800, 600);

  SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap != nullptr);
  REQUIRE(snap->layers().size() == 4);
}

TEST_CASE("SceneBuilder layer entries carry feature metadata", "[scene_builder]")
{
  auto ds = makeTestDataset();
  auto vs = makeViewport(0.0, 52.5, 5000000.0, 1200, 900);

  SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(ds, vs);
  REQUIRE(snap->layers().size() == 4);

  // Feature 0: point, classCode=100
  REQUIRE(snap->layers()[0].layerId == 100);
  REQUIRE(snap->layers()[0].featureIndex == 0);
  REQUIRE(snap->layers()[0].geometryType == static_cast<std::uint8_t>(GeometryType::kPoint));

  // Feature 1: line, classCode=200
  REQUIRE(snap->layers()[1].layerId == 200);
  REQUIRE(snap->layers()[1].featureIndex == 1);
  REQUIRE(snap->layers()[1].geometryType == static_cast<std::uint8_t>(GeometryType::kLine));

  // Feature 2: area, classCode=300
  REQUIRE(snap->layers()[2].layerId == 300);
  REQUIRE(snap->layers()[2].featureIndex == 2);
  REQUIRE(snap->layers()[2].geometryType == static_cast<std::uint8_t>(GeometryType::kArea));
}

TEST_CASE("SceneBuilder SENC roundtrip scene build", "[scene_builder][senc]")
{
  // Create dataset, write to SENC, read back, build scene -- full pipeline.
  auto srcDataset = makeTestDataset();

  SencWriter writer;
  auto blob = writer.write(srcDataset);
  REQUIRE(blob.size() > 0);

  SencReader reader;
  auto readResult = reader.read(blob);
  REQUIRE(readResult.ok);

  auto vs = makeViewport(0.0, 52.5, 5000000.0, 1200, 900);
  SceneBuilderFromSenc builder;
  auto snap = builder.buildAll(readResult.dataset, vs);
  REQUIRE(snap != nullptr);
  REQUIRE(snap->layers().size() == srcDataset.featureCount());
}
