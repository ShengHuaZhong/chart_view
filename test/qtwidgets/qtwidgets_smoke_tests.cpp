#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chart_view/qtwidgets/chart_view_widget.hpp>

#include "senc/senc_writer.hpp"
#include "chart_data/feature_chart_dataset.hpp"
#include "chart_data/geometry.hpp"

#include <QApplication>
#include <QByteArray>
#include <QColor>
#include <QImage>
#include <QWheelEvent>
#include <QtGlobal>

#include <string>
#include <vector>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#include <cstdlib>

namespace {
struct DebugCrtReportRedirect
{
  DebugCrtReportRedirect()
  {
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
  }
} g_debugCrtReportRedirect;
}// namespace
#endif

namespace {
QApplication &chart_view_test_application()
{
  static bool platform_initialized = [] {
    if(qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
      qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    }

    return true;
  }();

  static int argc = 1;
  static char application_name[] = "qtwidgets_smoke_tests";
  static char *argv[] = { application_name, nullptr };
  static QApplication app(argc, argv);

  (void)platform_initialized;
  return app;
}
}// namespace

TEST_CASE("ChartViewWidget bootstraps chart_runtime", "[qtwidgets]")
{
  auto &app = chart_view_test_application();
  chart_view::qtwidgets::ChartViewWidget widget;

  widget.resize(320, 240);
  widget.show();
  app.processEvents();

  REQUIRE(widget.hasRuntime());
  REQUIRE(widget.runtimeHandle() != nullptr);
  REQUIRE(widget.runtimeInfo().abi_version == CHART_VIEW_RUNTIME_ABI_VERSION);
  REQUIRE_FALSE(widget.statusText().isEmpty());

  widget.hide();
}

TEST_CASE("ChartViewWidget can attach an external runtime", "[qtwidgets]")
{
  auto &app = chart_view_test_application();

  // Create a runtime externally.
  chart_view_runtime_t *ext_runtime = nullptr;
  REQUIRE(chart_view_runtime_create(&ext_runtime) == chart_view_status_ok);

  chart_view::qtwidgets::ChartViewWidget widget;
  widget.resize(320, 240);
  widget.show();
  app.processEvents();

  // Detach the self-owned runtime and attach the external one.
  widget.detachRuntime();
  REQUIRE_FALSE(widget.hasRuntime());

  REQUIRE(widget.attachRuntime(ext_runtime));
  REQUIRE(widget.hasRuntime());
  REQUIRE(widget.runtimeHandle() == ext_runtime);

  // Detaching should NOT destroy the external runtime.
  widget.detachRuntime();
  REQUIRE_FALSE(widget.hasRuntime());

  // The external runtime should still be usable.
  chart_view_runtime_info_t info{};
  REQUIRE(chart_view_runtime_get_info(ext_runtime, &info) == chart_view_status_ok);
  REQUIRE(info.abi_version == CHART_VIEW_RUNTIME_ABI_VERSION);

  chart_view_runtime_destroy(ext_runtime);
  widget.hide();
}

TEST_CASE("ChartViewWidget rejects null attach", "[qtwidgets]")
{
  auto &app = chart_view_test_application();
  chart_view::qtwidgets::ChartViewWidget widget;

  widget.show();
  app.processEvents();

  REQUIRE_FALSE(widget.attachRuntime(nullptr));
  REQUIRE(widget.hasRuntime());// still has the self-owned runtime

  widget.hide();
}

TEST_CASE("ChartViewWidget bridges Phase 5 mariner settings and filters", "[qtwidgets]")
{
  auto &app = chart_view_test_application();
  chart_view::qtwidgets::ChartViewWidget widget;

  widget.resize(320, 240);
  widget.show();
  app.processEvents();

  REQUIRE(widget.hasRuntime());
  REQUIRE(chart_view_runtime_initialize(widget.runtimeHandle()) == chart_view_status_ok);

  chart_view_s52_mariner_settings_t settings{};
  REQUIRE(widget.s52MarinerSettings(&settings) == chart_view_status_ok);

  settings.palette = chart_view_s52_palette_night;
  settings.display_category = chart_view_s52_display_all;
  settings.show_text = 0U;
  settings.show_soundings = 0U;
  settings.simplified_points = 1U;
  settings.two_shades = 1U;
  settings.shallow_pattern = 0U;
  settings.full_sector_lights = 1U;
  settings.symbolized_boundaries = 0U;
  settings.honor_scamin = 0U;

  REQUIRE(widget.setS52MarinerSettings(settings) == chart_view_status_ok);

  chart_view_s52_mariner_settings_t roundtrip{};
  REQUIRE(widget.s52MarinerSettings(&roundtrip) == chart_view_status_ok);
  REQUIRE(roundtrip.palette == chart_view_s52_palette_night);
  REQUIRE(roundtrip.display_category == chart_view_s52_display_all);
  REQUIRE(roundtrip.show_text == 0U);
  REQUIRE(roundtrip.show_soundings == 0U);
  REQUIRE(roundtrip.simplified_points == 1U);
  REQUIRE(roundtrip.two_shades == 1U);
  REQUIRE(roundtrip.shallow_pattern == 0U);
  REQUIRE(roundtrip.full_sector_lights == 1U);
  REQUIRE(roundtrip.symbolized_boundaries == 0U);
  REQUIRE(roundtrip.honor_scamin == 0U);

  const chart_view_s57_class_filter_t classFilters[]{
    {"DEPARE", 0U},
    {"LIGHTS", 1U},
    {"WRECKS", 1U},
  };
  REQUIRE(widget.setS57ClassFilters(classFilters, 3U) == chart_view_status_ok);

  std::uint32_t classFilterCount = 0;
  REQUIRE(widget.s57ClassFilters(nullptr, &classFilterCount) == chart_view_status_ok);
  REQUIRE(classFilterCount >= 3U);

  std::vector<chart_view_s57_class_filter_t> classFilterRoundtrip(classFilterCount);
  REQUIRE(widget.s57ClassFilters(classFilterRoundtrip.data(), &classFilterCount) == chart_view_status_ok);

  bool depareDisabled = false;
  for(std::uint32_t index = 0; index < classFilterCount; ++index) {
    if(QString::fromUtf8(classFilterRoundtrip[index].object_acronym) == QStringLiteral("DEPARE")) {
      depareDisabled = classFilterRoundtrip[index].enabled == 0U;
      break;
    }
  }
  REQUIRE(depareDisabled);

  std::uint32_t ruleCount = 0;
  REQUIRE(widget.enumerateS52Rules(nullptr, &ruleCount) == chart_view_status_ok);
  REQUIRE(ruleCount > 0U);

  std::vector<chart_view_s52_rule_descriptor_t> rules(ruleCount);
  REQUIRE(widget.enumerateS52Rules(rules.data(), &ruleCount) == chart_view_status_ok);
  REQUIRE(ruleCount > 0U);
  REQUIRE(rules.front().rule_id != nullptr);

  const std::string firstRuleId = rules.front().rule_id;
  const chart_view_s52_rule_filter_t ruleFilters[]{
    {firstRuleId.c_str(), 0U},
  };
  REQUIRE(widget.setS52RuleFilters(ruleFilters, 1U) == chart_view_status_ok);

  std::uint32_t ruleFilterCount = 0;
  REQUIRE(widget.s52RuleFilters(nullptr, &ruleFilterCount) == chart_view_status_ok);
  REQUIRE(ruleFilterCount >= 1U);

  std::vector<chart_view_s52_rule_filter_t> ruleFilterRoundtrip(ruleFilterCount);
  REQUIRE(widget.s52RuleFilters(ruleFilterRoundtrip.data(), &ruleFilterCount) == chart_view_status_ok);

  bool foundDisabledRule = false;
  for(std::uint32_t index = 0; index < ruleFilterCount; ++index) {
    if(ruleFilterRoundtrip[index].rule_id != nullptr
       && QString::fromUtf8(ruleFilterRoundtrip[index].rule_id) == QString::fromStdString(firstRuleId)) {
      foundDisabledRule = ruleFilterRoundtrip[index].enabled == 0U;
      break;
    }
  }
  REQUIRE(foundDisabledRule);

  widget.hide();
}

TEST_CASE("ChartViewWidget render frame pipeline", "[qtwidgets]")
{
  using namespace chart_view::runtime::chart_data;
  using namespace chart_view::runtime::senc;

  auto &app = chart_view_test_application();

  // Build a small SENC blob.
  FeatureChartDataset ds;
  DatasetMeta meta;
  meta.name = "widget_test";
  meta.sourceType = chart_view_chart_source_s57;
  meta.extent = {-1.0, 50.0, 1.0, 52.0};
  ds.setMeta(std::move(meta));

  Feature f0;
  f0.id = 1;
  f0.classCode = 100;
  f0.geometry = PointGeometry{{0.0, 51.0}};
  ds.addFeature(std::move(f0));

  Feature f1;
  f1.id = 2;
  f1.classCode = 200;
  f1.geometry = AreaGeometry{{{-0.6, 50.6}, {0.6, 50.6}, {0.6, 51.4}, {-0.6, 51.4}}, {}};
  ds.addFeature(std::move(f1));

  SencWriter writer;
  auto senc = writer.write(ds);
  REQUIRE(senc.size() > 0);

  // Create widget and wire it up.
  chart_view::qtwidgets::ChartViewWidget widget;

  REQUIRE(widget.hasRuntime());

  // Initialize the runtime before resize/show so viewport set works.
  REQUIRE(chart_view_runtime_initialize(widget.runtimeHandle()) == chart_view_status_ok);

  widget.resize(800, 600);
  widget.show();
  app.processEvents();

  // Load SENC into the widget.
  auto loadStatus = widget.loadSenc(senc.data(), static_cast<std::uint32_t>(senc.size()));
  REQUIRE(loadStatus == chart_view_status_ok);

  // Ensure the viewport covers the test feature near lon/lat (0, 51).
  chart_view_viewport_t viewport{};
  viewport.center_lon = 0.0;
  viewport.center_lat = 51.0;
  viewport.scale_denominator = 100000.0;
  viewport.rotation_rad = 0.0;
  viewport.pixel_width = 800;
  viewport.pixel_height = 600;
  REQUIRE(chart_view_runtime_set_viewport(widget.runtimeHandle(), &viewport) == chart_view_status_ok);

  widget.resize(640, 480);
  app.processEvents();

  chart_view_viewport_t resizedViewport{};
  REQUIRE(chart_view_runtime_get_viewport(widget.runtimeHandle(), &resizedViewport) == chart_view_status_ok);
  REQUIRE(resizedViewport.center_lon == Catch::Approx(0.0));
  REQUIRE(resizedViewport.center_lat == Catch::Approx(51.0));
  REQUIRE(resizedViewport.pixel_width == widget.width());
  REQUIRE(resizedViewport.pixel_height == widget.height());

  // Trigger repaint to drive the render frame.
  widget.repaint();
  app.processEvents();

  auto result = widget.lastRenderResult();
  // At least one feature should have been rendered.
  REQUIRE(result.points_rendered >= 1);
  REQUIRE(result.areas_rendered >= 1);

  const auto grabbed = widget.grab().toImage().convertToFormat(QImage::Format_RGBA8888);
  REQUIRE_FALSE(grabbed.isNull());
  const auto centerColor = grabbed.pixelColor(grabbed.width() / 2, grabbed.height() / 2);
  REQUIRE(centerColor != QColor(230, 230, 217));

  widget.hide();
}

TEST_CASE("ChartViewWidget wheel zoom updates viewport scale and preserves anchor behavior", "[qtwidgets]")
{
  using namespace chart_view::runtime::chart_data;
  using namespace chart_view::runtime::senc;

  auto &app = chart_view_test_application();

  FeatureChartDataset ds;
  DatasetMeta meta;
  meta.name = "widget_zoom_test";
  meta.sourceType = chart_view_chart_source_s57;
  meta.extent = {-1.0, 50.0, 1.0, 52.0};
  ds.setMeta(std::move(meta));

  Feature feature;
  feature.id = 1;
  feature.classCode = 100;
  feature.geometry = PointGeometry{{0.02, 51.0}};
  ds.addFeature(std::move(feature));

  SencWriter writer;
  auto senc = writer.write(ds);
  REQUIRE(senc.size() > 0);

  chart_view::qtwidgets::ChartViewWidget widget;
  REQUIRE(widget.hasRuntime());
  REQUIRE(chart_view_runtime_initialize(widget.runtimeHandle()) == chart_view_status_ok);

  widget.resize(800, 600);
  widget.show();
  app.processEvents();

  REQUIRE(widget.loadSenc(senc.data(), static_cast<std::uint32_t>(senc.size())) == chart_view_status_ok);

  chart_view_viewport_t viewport{};
  viewport.center_lon = 0.0;
  viewport.center_lat = 51.0;
  viewport.scale_denominator = 100000.0;
  viewport.pixel_width = 800;
  viewport.pixel_height = 600;
  REQUIRE(chart_view_runtime_set_viewport(widget.runtimeHandle(), &viewport) == chart_view_status_ok);

  const QPointF centerPos(widget.width() / 2.0, widget.height() / 2.0);
  QWheelEvent centerZoom(
    centerPos,
    centerPos,
    QPoint(),
    QPoint(0, 120),
    Qt::NoButton,
    Qt::NoModifier,
    Qt::NoScrollPhase,
    false);
  QApplication::sendEvent(&widget, &centerZoom);
  app.processEvents();

  chart_view_viewport_t afterCenterZoom{};
  REQUIRE(chart_view_runtime_get_viewport(widget.runtimeHandle(), &afterCenterZoom) == chart_view_status_ok);
  REQUIRE(afterCenterZoom.scale_denominator == Catch::Approx(80000.0));
  REQUIRE(afterCenterZoom.center_lon == Catch::Approx(0.0).margin(0.01));
  REQUIRE(afterCenterZoom.center_lat == Catch::Approx(51.0).margin(0.01));

  const QPointF rightAnchor(widget.width() * 0.75, widget.height() * 0.5);
  QWheelEvent anchorZoom(
    rightAnchor,
    rightAnchor,
    QPoint(),
    QPoint(0, 120),
    Qt::NoButton,
    Qt::NoModifier,
    Qt::NoScrollPhase,
    false);
  QApplication::sendEvent(&widget, &anchorZoom);
  app.processEvents();

  chart_view_viewport_t afterAnchorZoom{};
  REQUIRE(chart_view_runtime_get_viewport(widget.runtimeHandle(), &afterAnchorZoom) == chart_view_status_ok);
  REQUIRE(afterAnchorZoom.scale_denominator == Catch::Approx(64000.0));
  REQUIRE(afterAnchorZoom.center_lon > afterCenterZoom.center_lon);

  widget.repaint();
  app.processEvents();
  const auto result = widget.lastRenderResult();
  REQUIRE(result.points_rendered >= 1);

  widget.hide();
}
