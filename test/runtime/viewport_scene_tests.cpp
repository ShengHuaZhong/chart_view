#include <catch2/catch_test_macros.hpp>

// Internal runtime headers (via target_include_directories)
#include "viewport_state.hpp"
#include "scene_model.hpp"
#include "scene_snapshot.hpp"
#include "render_scheduler.hpp"

#include <memory>

TEST_CASE("ViewportState default is invalid and at revision 0", "[runtime][viewport]")
{
  chart_view::runtime::ViewportState vs;
  REQUIRE(vs.revision() == 0);
  REQUIRE_FALSE(vs.isValid());
  REQUIRE(vs.viewport().pixel_width == 0);
}

TEST_CASE("ViewportState::set updates value and bumps revision", "[runtime][viewport]")
{
  chart_view::runtime::ViewportState vs;
  chart_view_viewport_t vp{};
  vp.center_lon = 121.5;
  vp.center_lat = 31.2;
  vp.scale_denominator = 50000.0;
  vp.rotation_rad = 0.0;
  vp.pixel_width = 1280;
  vp.pixel_height = 720;

  vs.set(vp);
  REQUIRE(vs.revision() == 1);
  REQUIRE(vs.isValid());
  REQUIRE(vs.viewport().center_lon == 121.5);
  REQUIRE(vs.viewport().pixel_width == 1280);

  vp.scale_denominator = 25000.0;
  vs.set(vp);
  REQUIRE(vs.revision() == 2);
  REQUIRE(vs.viewport().scale_denominator == 25000.0);
}

TEST_CASE("SceneModel starts empty", "[runtime][scene]")
{
  chart_view::runtime::SceneModel model;
  REQUIRE(model.empty());
  REQUIRE(model.layerCount() == 0);
}

TEST_CASE("SceneModel add and clear layers", "[runtime][scene]")
{
  chart_view::runtime::SceneModel model;
  model.addLayer({1});
  model.addLayer({2});
  REQUIRE(model.layerCount() == 2);
  REQUIRE_FALSE(model.empty());
  REQUIRE(model.layers()[0].layerId == 1);
  REQUIRE(model.layers()[1].layerId == 2);

  model.clear();
  REQUIRE(model.empty());
}

TEST_CASE("SceneSnapshot captures model and viewport state", "[runtime][scene]")
{
  chart_view::runtime::SceneModel model;
  model.addLayer({10});

  chart_view::runtime::ViewportState vs;
  chart_view_viewport_t vp{};
  vp.center_lon = 0.0;
  vp.center_lat = 0.0;
  vp.scale_denominator = 100000.0;
  vp.pixel_width = 800;
  vp.pixel_height = 600;
  vs.set(vp);

  chart_view::runtime::SceneSnapshot snap(model, vs);
  REQUIRE_FALSE(snap.empty());
  REQUIRE(snap.layers().size() == 1);
  REQUIRE(snap.layers()[0].layerId == 10);
  REQUIRE(snap.viewport().pixel_width == 800);
  REQUIRE(snap.viewportRevision() == 1);
}

TEST_CASE("RenderScheduler starts with no pending work", "[runtime][scheduler]")
{
  chart_view::runtime::RenderScheduler sched;
  REQUIRE_FALSE(sched.hasPendingWork());
  REQUIRE(sched.frameCount() == 0);
  REQUIRE(sched.latestSnapshot() == nullptr);
}

TEST_CASE("RenderScheduler submitSnapshot increments frame count", "[runtime][scheduler]")
{
  chart_view::runtime::RenderScheduler sched;

  chart_view::runtime::SceneModel model;
  chart_view::runtime::ViewportState vs;
  chart_view_viewport_t vp{};
  vp.scale_denominator = 50000.0;
  vp.pixel_width = 640;
  vp.pixel_height = 480;
  vs.set(vp);

  auto snap = std::make_shared<const chart_view::runtime::SceneSnapshot>(model, vs);
  sched.submitSnapshot(snap);

  REQUIRE(sched.hasPendingWork());
  REQUIRE(sched.frameCount() == 1);
  REQUIRE(sched.latestSnapshot() == snap);

  auto snap2 = std::make_shared<const chart_view::runtime::SceneSnapshot>(model, vs);
  sched.submitSnapshot(snap2);
  REQUIRE(sched.frameCount() == 2);
  REQUIRE(sched.latestSnapshot() == snap2);
}
