#include "main_window.hpp"

#include <chart_view/qtwidgets/chart_view_widget.hpp>
#include <chart_view/runtime/chart_runtime.h>

#include <QAction>
#include <QFile>
#include <QFileDialog>
#include <QDebug>
#include <QFileInfo>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

namespace {
chart_view_chart_source_type_t sourceTypeFromPath(const QString &path)
{
  const auto suffix = QFileInfo(path).suffix().toLower();

  if(suffix == QStringLiteral("c") || suffix == QStringLiteral("cm93")) {
    return chart_view_chart_source_cm93;
  }

  if(suffix == QStringLiteral("s101") || suffix == QStringLiteral("101")
     || suffix == QStringLiteral("gml") || suffix == QStringLiteral("xml")) {
    return chart_view_chart_source_s101;
  }

  if(suffix == QStringLiteral("000") || suffix == QStringLiteral("001")
     || suffix == QStringLiteral("002") || suffix == QStringLiteral("s57")) {
    return chart_view_chart_source_s57;
  }

  return chart_view_chart_source_unknown;
}

chart_view_chart_source_type_t sourceTypeFromFilter(const QString &filter)
{
  if(filter.startsWith(QStringLiteral("S-57"))) {
    return chart_view_chart_source_s57;
  }
  if(filter.startsWith(QStringLiteral("CM93"))) {
    return chart_view_chart_source_cm93;
  }
  if(filter.startsWith(QStringLiteral("S-101"))) {
    return chart_view_chart_source_s101;
  }
  return chart_view_chart_source_unknown;
}

chart_view_chart_source_type_t resolveSourceType(const QString &path, const QString &filter)
{
  const auto fromFilter = sourceTypeFromFilter(filter);
  return fromFilter == chart_view_chart_source_unknown ? sourceTypeFromPath(path) : fromFilter;
}

QString sourceTypeName(chart_view_chart_source_type_t sourceType)
{
  switch(sourceType) {
  case chart_view_chart_source_s57:
    return QStringLiteral("S-57");
  case chart_view_chart_source_cm93:
    return QStringLiteral("CM93");
  case chart_view_chart_source_s101:
    return QStringLiteral("S-101");
  case chart_view_chart_source_unknown:
  default:
    return QStringLiteral("unknown");
  }
}
}// namespace

namespace chart_standalone {

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
{
  setWindowTitle(QStringLiteral("chart_standalone"));
  resize(1280, 720);

  m_chartWidget = new chart_view::qtwidgets::ChartViewWidget(this);
  setCentralWidget(m_chartWidget);

  createMenus();
  createStatusBar();
}

MainWindow::~MainWindow() = default;

chart_view::qtwidgets::ChartViewWidget *MainWindow::chartWidget() const noexcept
{
  return m_chartWidget;
}

bool MainWindow::openChartFile(const QString &path, chart_view_chart_source_type_t sourceType)
{
  if(path.isEmpty() || m_chartWidget == nullptr || !m_chartWidget->hasRuntime()) {
    qWarning() << "open_chart_failed reason=precondition path_empty=" << path.isEmpty()
               << "widget_null=" << (m_chartWidget == nullptr)
               << "no_runtime=" << (m_chartWidget != nullptr && !m_chartWidget->hasRuntime());
    return false;
  }

  const auto resolvedType = sourceType == chart_view_chart_source_unknown
                              ? sourceTypeFromPath(path)
                              : sourceType;
  if(resolvedType == chart_view_chart_source_unknown) {
    statusBar()->showMessage(QStringLiteral("Cannot infer chart format for %1").arg(path), 5000);
    qWarning() << "open_chart_failed reason=unknown_source path=" << path;
    return false;
  }

  auto *runtime = m_chartWidget->runtimeHandle();
  const auto initStatus = chart_view_runtime_initialize(runtime);
  if(initStatus != chart_view_status_ok && initStatus != chart_view_status_already_initialized) {
    statusBar()->showMessage(
      QStringLiteral("Runtime initialize failed (%1)").arg(static_cast<int>(initStatus)),
      5000);
    qWarning() << "open_chart_failed reason=runtime_init status=" << static_cast<int>(initStatus);
    return false;
  }

  const auto nativePath = QFile::encodeName(path);
  const auto loadStatus = chart_view_runtime_open_chart_file(runtime, nativePath.constData(), resolvedType);
  if(loadStatus != chart_view_status_ok) {
    statusBar()->showMessage(
      QStringLiteral("Open chart failed (%1)").arg(static_cast<int>(loadStatus)),
      5000);
    qWarning() << "open_chart_failed reason=runtime_open status=" << static_cast<int>(loadStatus)
               << "path=" << path << "source=" << sourceTypeName(resolvedType);
    return false;
  }

  m_chartWidget->update();
  statusBar()->showMessage(
    QStringLiteral("Opened %1 as %2").arg(QFileInfo(path).fileName(), sourceTypeName(resolvedType)),
    5000);
  return true;
}

void MainWindow::createMenus()
{
  auto *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));

  auto *openAction = fileMenu->addAction(QStringLiteral("&Open Chart..."));
  openAction->setShortcut(QKeySequence::Open);
  connect(openAction, &QAction::triggered, this, &MainWindow::openChartDialog);

  fileMenu->addAction(QStringLiteral("E&xit"), this, &QWidget::close);

  auto *helpMenu = menuBar()->addMenu(QStringLiteral("&Help"));
  helpMenu->addAction(QStringLiteral("&About"), this, [this]() {
    statusBar()->showMessage(
      QString::fromLatin1("chart_standalone %1")
        .arg(QString::fromUtf8(chart_view_runtime_version_string())),
      3000);
  });
}

void MainWindow::createStatusBar()
{
  statusBar()->showMessage(QStringLiteral("Ready"));
}

void MainWindow::openChartDialog()
{
  const QString filters = QStringLiteral(
    "Supported Charts (*.000 *.001 *.002 *.S57 *.s57 *.C *.c *.cm93 *.CM93 *.S101 *.s101 *.101 *.gml *.xml);;"
    "S-57 Charts (*.000 *.001 *.002 *.S57 *.s57);;"
    "CM93 Cells (*.C *.c *.cm93 *.CM93);;"
    "S-101 Datasets (*.S101 *.s101 *.101 *.gml *.xml);;"
    "All Files (*)");

  QString selectedFilter;
  const auto path = QFileDialog::getOpenFileName(
    this,
    QStringLiteral("Open Chart"),
    QString(),
    filters,
    &selectedFilter);

  if(path.isEmpty()) {
    return;
  }

  const auto sourceType = resolveSourceType(path, selectedFilter);
  if(!openChartFile(path, sourceType)) {
    QMessageBox::warning(
      this,
      QStringLiteral("Open Chart"),
      QStringLiteral("Failed to open chart file:\n%1").arg(path));
  }
}

}// namespace chart_standalone
