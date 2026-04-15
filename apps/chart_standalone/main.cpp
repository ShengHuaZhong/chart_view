#include "main_window.hpp"

#include <chart_view/qtwidgets/chart_view_widget.hpp>
#include <chart_view/runtime/chart_runtime.h>

#include <QApplication>
#include <QDebug>

#include <cstdlib>
#include <iostream>

namespace {
bool chart_view_has_arg(int argc, char *argv[], const char *needle)
{
  for(int i = 1; i < argc; ++i) {
    if(QString::fromLocal8Bit(argv[i]) == QString::fromLatin1(needle)) {
      return true;
    }
  }

  return false;
}

QString chart_view_arg_value(int argc, char *argv[], const char *needle)
{
  for(int i = 1; i + 1 < argc; ++i) {
    if(QString::fromLocal8Bit(argv[i]) == QString::fromLatin1(needle)) {
      return QString::fromLocal8Bit(argv[i + 1]);
    }
  }
  return {};
}

chart_view_chart_source_type_t chart_view_parse_source_type(const QString &value)
{
  const auto lowered = value.toLower();
  if(lowered == QStringLiteral("s57")) {
    return chart_view_chart_source_s57;
  }
  if(lowered == QStringLiteral("cm93")) {
    return chart_view_chart_source_cm93;
  }
  if(lowered == QStringLiteral("s101")) {
    return chart_view_chart_source_s101;
  }
  return chart_view_chart_source_unknown;
}

int chart_view_run_smoke_test(QApplication &app)
{
  chart_view::qtwidgets::ChartViewWidget widget;
  widget.resize(640, 360);
  widget.show();
  app.processEvents();
  widget.hide();
  return EXIT_SUCCESS;
}

const char *chart_view_source_name(chart_view_chart_source_type_t sourceType)
{
  switch(sourceType) {
  case chart_view_chart_source_s57:
    return "s57";
  case chart_view_chart_source_cm93:
    return "cm93";
  case chart_view_chart_source_s101:
    return "s101";
  case chart_view_chart_source_unknown:
  default:
    return "unknown";
  }
}

int chart_view_run_open_chart_smoke(QApplication &app,
                                    const QString &path,
                                    chart_view_chart_source_type_t sourceType)
{
  chart_standalone::MainWindow window;
  window.resize(1280, 720);
  window.show();
  app.processEvents();

  if(!window.openChartFile(path, sourceType)) {
    qWarning() << "smoke_fail reason=openChartFile";
    return EXIT_FAILURE;
  }

  auto *widget = window.chartWidget();
  if(widget == nullptr || !widget->hasRuntime()) {
    qWarning() << "smoke_fail reason=no_runtime";
    return EXIT_FAILURE;
  }

  chart_view_loaded_chart_info_t chartInfo{};
  if(chart_view_runtime_get_loaded_chart_info(widget->runtimeHandle(), &chartInfo) != chart_view_status_ok) {
    qWarning() << "smoke_fail reason=get_loaded_chart_info";
    return EXIT_FAILURE;
  }

  if(sourceType != chart_view_chart_source_unknown && chartInfo.source_type != sourceType) {
    qWarning() << "smoke_fail reason=unexpected_loaded_chart_type"
               << "expected=" << chart_view_source_name(sourceType)
               << "actual=" << chart_view_source_name(chartInfo.source_type);
    return EXIT_FAILURE;
  }

  qInfo() << "loaded_chart"
          << "source=" << chart_view_source_name(chartInfo.source_type)
          << "feature_count=" << chartInfo.feature_count
          << "extent=[" << chartInfo.min_lon << "," << chartInfo.min_lat
          << "->" << chartInfo.max_lon << "," << chartInfo.max_lat << "]";

  chart_view_render_frame_result_t frameResult{};
  if(chart_view_runtime_render_frame(widget->runtimeHandle(), &frameResult) != chart_view_status_ok) {
    return EXIT_FAILURE;
  }

  qInfo() << "render_result"
          << "points=" << frameResult.points_rendered
          << "lines=" << frameResult.lines_rendered
          << "areas=" << frameResult.areas_rendered
          << "total_vertices=" << frameResult.total_vertices;

  const auto renderedGeometry = frameResult.points_rendered + frameResult.lines_rendered + frameResult.areas_rendered;
  // Phase 1 host smoke for CM93 validates open/load path and render invocation.
  // Full CM93 geometry extraction will be completed in a later task.
  if(chartInfo.source_type != chart_view_chart_source_s101 &&
     chartInfo.source_type != chart_view_chart_source_cm93 &&
     renderedGeometry == 0U) {
    return EXIT_FAILURE;
  }

  app.processEvents();
  window.hide();
  return EXIT_SUCCESS;
}
}// namespace

int main(int argc, char *argv[])
{
  if(chart_view_has_arg(argc, argv, "--version")) {
    std::cout << "chart_standalone " << chart_view_runtime_version_string() << '\n';
    return EXIT_SUCCESS;
  }

  const auto openChartPath = chart_view_arg_value(argc, argv, "--open-chart");
  const auto sourceArg = chart_view_arg_value(argc, argv, "--chart-type");

  QApplication app(argc, argv);

  if(!openChartPath.isEmpty()) {
    const auto sourceType = chart_view_parse_source_type(sourceArg);
    if(chart_view_has_arg(argc, argv, "--smoke-test")) {
      return chart_view_run_open_chart_smoke(app, openChartPath, sourceType);
    }

    chart_standalone::MainWindow window;
    window.show();
    if(!window.openChartFile(openChartPath, sourceType)) {
      return EXIT_FAILURE;
    }
    return QApplication::exec();
  }

  if(chart_view_has_arg(argc, argv, "--smoke-test")) {
    return chart_view_run_smoke_test(app);
  }

  chart_standalone::MainWindow window;
  window.show();

  return QApplication::exec();
}
