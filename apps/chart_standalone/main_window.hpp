#ifndef CHART_STANDALONE_MAIN_WINDOW_HPP
#define CHART_STANDALONE_MAIN_WINDOW_HPP

#include <chart_view/runtime/chart_runtime_types.h>

#include <QMainWindow>
#include <QString>

namespace chart_view::qtwidgets {
class ChartViewWidget;
}

class QCheckBox;
class QComboBox;
class QDockWidget;

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
  [[nodiscard]] bool applyPhase5DemoControlPreset();

private:
  void createMenus();
  void createStatusBar();
  void createPhase5Controls();
  void openChartDialog();
  void openChartDirectoryDialog();
  void syncPhase5ControlsFromRuntime();
  void refreshRuleFilterControls();
  void applyPhase5MarinerSettings();
  void applyPhase5ClassFilters();
  void applyPhase5RuleFilters();
  [[nodiscard]] bool ensureRuntimeInitialized();

  chart_view::qtwidgets::ChartViewWidget *m_chartWidget{nullptr};
  QDockWidget *m_phase5ControlsDock{nullptr};
  QComboBox *m_paletteCombo{nullptr};
  QComboBox *m_displayCategoryCombo{nullptr};
  QCheckBox *m_showTextCheck{nullptr};
  QCheckBox *m_showSoundingsCheck{nullptr};
  QCheckBox *m_simplifiedPointsCheck{nullptr};
  QCheckBox *m_twoShadesCheck{nullptr};
  QCheckBox *m_shallowPatternCheck{nullptr};
  QCheckBox *m_fullSectorLightsCheck{nullptr};
  QCheckBox *m_symbolizedBoundariesCheck{nullptr};
  QCheckBox *m_honorScaminCheck{nullptr};
  QCheckBox *m_classDepareCheck{nullptr};
  QCheckBox *m_classLightsCheck{nullptr};
  QCheckBox *m_classWrecksCheck{nullptr};
  QCheckBox *m_ruleDepareCheck{nullptr};
  QCheckBox *m_ruleWrecksCheck{nullptr};
  QString m_ruleDepareId;
  QString m_ruleWrecksId;
};

}// namespace chart_standalone

#endif
