#pragma once

#include <QMainWindow>

namespace chartsys::qtwidgets {
class ChartViewWidget;
}

class MainWindow final : public QMainWindow {
public:
  MainWindow();
  ~MainWindow() override;

private:
  chartsys::qtwidgets::ChartViewWidget* chart_view_{nullptr};
};
