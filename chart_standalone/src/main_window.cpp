#include "main_window.hpp"

#include "chart_qtwidgets/chart_view_widget.hpp"

#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>

MainWindow::MainWindow() {
  setWindowTitle("Chart Standalone");
  resize(1280, 720);

  chart_view_ = new chartsys::qtwidgets::ChartViewWidget(this);
  setCentralWidget(chart_view_);

  QMenu* file_menu = menuBar()->addMenu("&File");
  file_menu->addAction("E&xit", this, &QWidget::close);

  statusBar()->showMessage("Ready");
}

MainWindow::~MainWindow() = default;
