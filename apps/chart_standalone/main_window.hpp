#ifndef CHART_STANDALONE_MAIN_WINDOW_HPP
#define CHART_STANDALONE_MAIN_WINDOW_HPP

#include <chart_view/runtime/chart_runtime_types.h>

#include <QMainWindow>
#include <QString>

#include <memory>

namespace chart_view::qtwidgets {
class ChartViewWidget;
}

namespace chart_standalone {

class MainWindow final : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

  MainWindow(const MainWindow &) = delete;
  MainWindow &operator=(const MainWindow &) = delete;
  MainWindow(MainWindow &&) = delete;
  MainWindow &operator=(MainWindow &&) = delete;

  [[nodiscard]] chart_view::qtwidgets::ChartViewWidget *chartWidget() const noexcept;

  [[nodiscard]] bool openChartFile(const QString &path,
                                   chart_view_chart_source_type_t sourceType);
  [[nodiscard]] bool openChartDirectory(const QString &path);

private:
  void createMenus();
  void createStatusBar();
  void openChartDialog();
  void openChartDirectoryDialog();

  chart_view::qtwidgets::ChartViewWidget *m_chartWidget{nullptr};
};

}// namespace chart_standalone

#endif
