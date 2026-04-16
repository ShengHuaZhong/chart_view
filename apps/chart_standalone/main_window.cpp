#include "main_window.hpp"

#include <chart_view/qtwidgets/chart_view_widget.hpp>
#include <chart_view/runtime/chart_runtime.h>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QDebug>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QVBoxLayout>

#include <array>
#include <vector>

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

QString paletteName(chart_view_s52_color_palette_t palette)
{
  switch(palette) {
  case chart_view_s52_palette_dusk:
    return QStringLiteral("Dusk");
  case chart_view_s52_palette_night:
    return QStringLiteral("Night");
  case chart_view_s52_palette_day:
  default:
    return QStringLiteral("Day");
  }
}

QString displayCategoryName(chart_view_s52_display_category_t category)
{
  switch(category) {
  case chart_view_s52_display_base:
    return QStringLiteral("Display Base");
  case chart_view_s52_display_all:
    return QStringLiteral("All");
  case chart_view_s52_display_standard:
  default:
    return QStringLiteral("Standard");
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

  createPhase5Controls();
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
  syncPhase5ControlsFromRuntime();
  statusBar()->showMessage(
    QStringLiteral("Opened %1 as %2").arg(QFileInfo(path).fileName(), sourceTypeName(resolvedType)),
    5000);
  return true;
}

bool MainWindow::openChartDirectory(const QString &path)
{
  if(path.isEmpty() || m_chartWidget == nullptr || !m_chartWidget->hasRuntime()) {
    qWarning() << "open_chart_directory_failed reason=precondition path_empty=" << path.isEmpty()
               << "widget_null=" << (m_chartWidget == nullptr)
               << "no_runtime=" << (m_chartWidget != nullptr && !m_chartWidget->hasRuntime());
    return false;
  }

  const QFileInfo info(path);
  if(!info.exists() || !info.isDir()) {
    statusBar()->showMessage(QStringLiteral("Chart directory does not exist: %1").arg(path), 5000);
    qWarning() << "open_chart_directory_failed reason=not_directory path=" << path;
    return false;
  }

  auto *runtime = m_chartWidget->runtimeHandle();
  const auto initStatus = chart_view_runtime_initialize(runtime);
  if(initStatus != chart_view_status_ok && initStatus != chart_view_status_already_initialized) {
    statusBar()->showMessage(
      QStringLiteral("Runtime initialize failed (%1)").arg(static_cast<int>(initStatus)),
      5000);
    qWarning() << "open_chart_directory_failed reason=runtime_init status="
               << static_cast<int>(initStatus);
    return false;
  }

  const auto nativePath = QFile::encodeName(path);
  const auto openStatus = chart_view_runtime_open_chart_directory(runtime, nativePath.constData());
  if(openStatus != chart_view_status_ok) {
    statusBar()->showMessage(
      QStringLiteral("Open chart directory failed (%1)").arg(static_cast<int>(openStatus)),
      5000);
    qWarning() << "open_chart_directory_failed reason=runtime_open status="
               << static_cast<int>(openStatus) << "path=" << path;
    return false;
  }

  m_chartWidget->update();
  syncPhase5ControlsFromRuntime();
  const auto label = info.fileName().isEmpty() ? info.absoluteFilePath() : info.fileName();
  statusBar()->showMessage(
    QStringLiteral("Opened %1 in multi-chart mode").arg(label),
    5000);
  return true;
}

void MainWindow::createMenus()
{
  auto *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));

  auto *openAction = fileMenu->addAction(QStringLiteral("&Open Chart..."));
  openAction->setShortcut(QKeySequence::Open);
  connect(openAction, &QAction::triggered, this, &MainWindow::openChartDialog);

  auto *openDirectoryAction = fileMenu->addAction(QStringLiteral("Open Chart &Directory..."));
  connect(openDirectoryAction, &QAction::triggered, this, &MainWindow::openChartDirectoryDialog);

  fileMenu->addAction(QStringLiteral("E&xit"), this, &QWidget::close);

  auto *viewMenu = menuBar()->addMenu(QStringLiteral("&View"));
  if(m_phase5ControlsDock != nullptr) {
    auto *toggleControls = m_phase5ControlsDock->toggleViewAction();
    toggleControls->setText(QStringLiteral("Phase &5 Controls"));
    viewMenu->addAction(toggleControls);
  }

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

void MainWindow::createPhase5Controls()
{
  m_phase5ControlsDock = new QDockWidget(QStringLiteral("Phase 5 Controls"), this);
  m_phase5ControlsDock->setObjectName(QStringLiteral("phase5ControlsDock"));

  auto *container = new QWidget(m_phase5ControlsDock);
  auto *layout = new QVBoxLayout(container);

  auto *marinerGroup = new QGroupBox(QStringLiteral("S-52 Mariner Settings"), container);
  auto *marinerLayout = new QFormLayout(marinerGroup);

  m_paletteCombo = new QComboBox(marinerGroup);
  m_paletteCombo->setObjectName(QStringLiteral("s52PaletteCombo"));
  m_paletteCombo->addItem(QStringLiteral("Day"), static_cast<int>(chart_view_s52_palette_day));
  m_paletteCombo->addItem(QStringLiteral("Dusk"), static_cast<int>(chart_view_s52_palette_dusk));
  m_paletteCombo->addItem(QStringLiteral("Night"), static_cast<int>(chart_view_s52_palette_night));

  m_displayCategoryCombo = new QComboBox(marinerGroup);
  m_displayCategoryCombo->setObjectName(QStringLiteral("s52DisplayCategoryCombo"));
  m_displayCategoryCombo->addItem(QStringLiteral("Display Base"), static_cast<int>(chart_view_s52_display_base));
  m_displayCategoryCombo->addItem(QStringLiteral("Standard"), static_cast<int>(chart_view_s52_display_standard));
  m_displayCategoryCombo->addItem(QStringLiteral("All"), static_cast<int>(chart_view_s52_display_all));

  m_showTextCheck = new QCheckBox(QStringLiteral("Show text labels"), marinerGroup);
  m_showTextCheck->setObjectName(QStringLiteral("s52ShowTextCheck"));
  m_showTextCheck->setChecked(true);
  m_showSoundingsCheck = new QCheckBox(QStringLiteral("Show soundings"), marinerGroup);
  m_showSoundingsCheck->setObjectName(QStringLiteral("s52ShowSoundingsCheck"));
  m_showSoundingsCheck->setChecked(true);
  m_simplifiedPointsCheck = new QCheckBox(QStringLiteral("Simplified point symbols"), marinerGroup);
  m_simplifiedPointsCheck->setObjectName(QStringLiteral("s52SimplifiedPointsCheck"));
  m_twoShadesCheck = new QCheckBox(QStringLiteral("Two shades"), marinerGroup);
  m_twoShadesCheck->setObjectName(QStringLiteral("s52TwoShadesCheck"));
  m_shallowPatternCheck = new QCheckBox(QStringLiteral("Shallow pattern"), marinerGroup);
  m_shallowPatternCheck->setObjectName(QStringLiteral("s52ShallowPatternCheck"));
  m_shallowPatternCheck->setChecked(true);
  m_fullSectorLightsCheck = new QCheckBox(QStringLiteral("Full sector lights"), marinerGroup);
  m_fullSectorLightsCheck->setObjectName(QStringLiteral("s52FullSectorLightsCheck"));
  m_symbolizedBoundariesCheck = new QCheckBox(QStringLiteral("Symbolized boundaries"), marinerGroup);
  m_symbolizedBoundariesCheck->setObjectName(QStringLiteral("s52SymbolizedBoundariesCheck"));
  m_symbolizedBoundariesCheck->setChecked(true);
  m_honorScaminCheck = new QCheckBox(QStringLiteral("Honor SCAMIN"), marinerGroup);
  m_honorScaminCheck->setObjectName(QStringLiteral("s52HonorScaminCheck"));
  m_honorScaminCheck->setChecked(true);

  marinerLayout->addRow(QStringLiteral("Palette"), m_paletteCombo);
  marinerLayout->addRow(QStringLiteral("Display category"), m_displayCategoryCombo);
  marinerLayout->addRow(QString(), m_showTextCheck);
  marinerLayout->addRow(QString(), m_showSoundingsCheck);
  marinerLayout->addRow(QString(), m_simplifiedPointsCheck);
  marinerLayout->addRow(QString(), m_twoShadesCheck);
  marinerLayout->addRow(QString(), m_shallowPatternCheck);
  marinerLayout->addRow(QString(), m_fullSectorLightsCheck);
  marinerLayout->addRow(QString(), m_symbolizedBoundariesCheck);
  marinerLayout->addRow(QString(), m_honorScaminCheck);

  auto *classGroup = new QGroupBox(QStringLiteral("S57 Class Filters"), container);
  auto *classLayout = new QVBoxLayout(classGroup);
  m_classDepareCheck = new QCheckBox(QStringLiteral("DEPARE"), classGroup);
  m_classDepareCheck->setObjectName(QStringLiteral("classDepareCheck"));
  m_classDepareCheck->setChecked(true);
  m_classLightsCheck = new QCheckBox(QStringLiteral("LIGHTS"), classGroup);
  m_classLightsCheck->setObjectName(QStringLiteral("classLightsCheck"));
  m_classLightsCheck->setChecked(true);
  m_classWrecksCheck = new QCheckBox(QStringLiteral("WRECKS"), classGroup);
  m_classWrecksCheck->setObjectName(QStringLiteral("classWrecksCheck"));
  m_classWrecksCheck->setChecked(true);
  classLayout->addWidget(m_classDepareCheck);
  classLayout->addWidget(m_classLightsCheck);
  classLayout->addWidget(m_classWrecksCheck);

  auto *ruleGroup = new QGroupBox(QStringLiteral("Selected S-52 Rule Filters"), container);
  auto *ruleLayout = new QVBoxLayout(ruleGroup);
  m_ruleDepareCheck = new QCheckBox(QStringLiteral("DEPARE rule (not available yet)"), ruleGroup);
  m_ruleDepareCheck->setObjectName(QStringLiteral("ruleDepareCheck"));
  m_ruleWrecksCheck = new QCheckBox(QStringLiteral("WRECKS rule (not available yet)"), ruleGroup);
  m_ruleWrecksCheck->setObjectName(QStringLiteral("ruleWrecksCheck"));
  m_ruleDepareCheck->setEnabled(false);
  m_ruleWrecksCheck->setEnabled(false);
  ruleLayout->addWidget(m_ruleDepareCheck);
  ruleLayout->addWidget(m_ruleWrecksCheck);

  layout->addWidget(marinerGroup);
  layout->addWidget(classGroup);
  layout->addWidget(ruleGroup);
  layout->addStretch(1);
  container->setLayout(layout);

  m_phase5ControlsDock->setWidget(container);
  addDockWidget(Qt::RightDockWidgetArea, m_phase5ControlsDock);

  connect(m_paletteCombo, &QComboBox::currentIndexChanged, this, [this](int) {
    applyPhase5MarinerSettings();
  });
  connect(m_displayCategoryCombo, &QComboBox::currentIndexChanged, this, [this](int) {
    applyPhase5MarinerSettings();
  });

  for(auto *checkbox : std::array<QCheckBox *, 8>{
        m_showTextCheck,
        m_showSoundingsCheck,
        m_simplifiedPointsCheck,
        m_twoShadesCheck,
        m_shallowPatternCheck,
        m_fullSectorLightsCheck,
        m_symbolizedBoundariesCheck,
        m_honorScaminCheck}) {
    connect(checkbox, &QCheckBox::toggled, this, [this](bool) {
      applyPhase5MarinerSettings();
    });
  }

  for(auto *checkbox : std::array<QCheckBox *, 3>{
        m_classDepareCheck,
        m_classLightsCheck,
        m_classWrecksCheck}) {
    connect(checkbox, &QCheckBox::toggled, this, [this](bool) {
      applyPhase5ClassFilters();
    });
  }

  for(auto *checkbox : std::array<QCheckBox *, 2>{
        m_ruleDepareCheck,
        m_ruleWrecksCheck}) {
    connect(checkbox, &QCheckBox::toggled, this, [this](bool) {
      applyPhase5RuleFilters();
    });
  }
}

bool MainWindow::ensureRuntimeInitialized()
{
  if(m_chartWidget == nullptr || !m_chartWidget->hasRuntime()) {
    return false;
  }

  const auto status = chart_view_runtime_initialize(m_chartWidget->runtimeHandle());
  return status == chart_view_status_ok || status == chart_view_status_already_initialized;
}

void MainWindow::syncPhase5ControlsFromRuntime()
{
  if(m_chartWidget == nullptr || !ensureRuntimeInitialized()) {
    return;
  }

  chart_view_s52_mariner_settings_t settings{};
  if(m_chartWidget->s52MarinerSettings(&settings) == chart_view_status_ok) {
    const QSignalBlocker paletteBlocker(m_paletteCombo);
    const QSignalBlocker categoryBlocker(m_displayCategoryCombo);
    const QSignalBlocker textBlocker(m_showTextCheck);
    const QSignalBlocker soundingBlocker(m_showSoundingsCheck);
    const QSignalBlocker simplifiedBlocker(m_simplifiedPointsCheck);
    const QSignalBlocker twoShadesBlocker(m_twoShadesCheck);
    const QSignalBlocker shallowPatternBlocker(m_shallowPatternCheck);
    const QSignalBlocker fullSectorBlocker(m_fullSectorLightsCheck);
    const QSignalBlocker boundariesBlocker(m_symbolizedBoundariesCheck);
    const QSignalBlocker scaminBlocker(m_honorScaminCheck);

    m_paletteCombo->setCurrentIndex(m_paletteCombo->findData(static_cast<int>(settings.palette)));
    m_displayCategoryCombo->setCurrentIndex(
      m_displayCategoryCombo->findData(static_cast<int>(settings.display_category)));
    m_showTextCheck->setChecked(settings.show_text != 0U);
    m_showSoundingsCheck->setChecked(settings.show_soundings != 0U);
    m_simplifiedPointsCheck->setChecked(settings.simplified_points != 0U);
    m_twoShadesCheck->setChecked(settings.two_shades != 0U);
    m_shallowPatternCheck->setChecked(settings.shallow_pattern != 0U);
    m_fullSectorLightsCheck->setChecked(settings.full_sector_lights != 0U);
    m_symbolizedBoundariesCheck->setChecked(settings.symbolized_boundaries != 0U);
    m_honorScaminCheck->setChecked(settings.honor_scamin != 0U);
  }

  bool depareEnabled = true;
  bool lightsEnabled = true;
  bool wrecksEnabled = true;
  std::uint32_t classFilterCount = 0;
  if(m_chartWidget->s57ClassFilters(nullptr, &classFilterCount) == chart_view_status_ok && classFilterCount > 0U) {
    std::vector<chart_view_s57_class_filter_t> filters(classFilterCount);
    if(m_chartWidget->s57ClassFilters(filters.data(), &classFilterCount) == chart_view_status_ok) {
      for(std::uint32_t index = 0; index < classFilterCount; ++index) {
        const QString acronym = QString::fromUtf8(filters[index].object_acronym == nullptr ? "" : filters[index].object_acronym);
        if(acronym == QStringLiteral("DEPARE")) {
          depareEnabled = filters[index].enabled != 0U;
        } else if(acronym == QStringLiteral("LIGHTS")) {
          lightsEnabled = filters[index].enabled != 0U;
        } else if(acronym == QStringLiteral("WRECKS")) {
          wrecksEnabled = filters[index].enabled != 0U;
        }
      }
    }
  }

  {
    const QSignalBlocker depareBlocker(m_classDepareCheck);
    const QSignalBlocker lightsBlocker(m_classLightsCheck);
    const QSignalBlocker wrecksBlocker(m_classWrecksCheck);
    m_classDepareCheck->setChecked(depareEnabled);
    m_classLightsCheck->setChecked(lightsEnabled);
    m_classWrecksCheck->setChecked(wrecksEnabled);
  }

  refreshRuleFilterControls();
}

void MainWindow::refreshRuleFilterControls()
{
  if(m_chartWidget == nullptr || !ensureRuntimeInitialized()) {
    return;
  }

  m_ruleDepareId.clear();
  m_ruleWrecksId.clear();

  std::uint32_t ruleCount = 0;
  if(m_chartWidget->enumerateS52Rules(nullptr, &ruleCount) != chart_view_status_ok || ruleCount == 0U) {
    const QSignalBlocker depareBlocker(m_ruleDepareCheck);
    const QSignalBlocker wrecksBlocker(m_ruleWrecksCheck);
    m_ruleDepareCheck->setText(QStringLiteral("DEPARE rule (not available yet)"));
    m_ruleDepareCheck->setChecked(true);
    m_ruleDepareCheck->setEnabled(false);
    m_ruleWrecksCheck->setText(QStringLiteral("WRECKS rule (not available yet)"));
    m_ruleWrecksCheck->setChecked(true);
    m_ruleWrecksCheck->setEnabled(false);
    return;
  }

  std::vector<chart_view_s52_rule_descriptor_t> rules(ruleCount);
  if(m_chartWidget->enumerateS52Rules(rules.data(), &ruleCount) != chart_view_status_ok) {
    return;
  }

  auto assignRule = [&](const QString &targetAcronym, QString &outRuleId, QCheckBox *checkbox) {
    for(std::uint32_t index = 0; index < ruleCount; ++index) {
      const QString objectAcronym = QString::fromUtf8(
        rules[index].object_acronym == nullptr ? "" : rules[index].object_acronym);
      if(objectAcronym != targetAcronym) {
        continue;
      }

      outRuleId = QString::fromUtf8(rules[index].rule_id == nullptr ? "" : rules[index].rule_id);
      const auto label = QString::fromUtf8(rules[index].label == nullptr ? "" : rules[index].label);
      checkbox->setText(
        label.isEmpty()
          ? QStringLiteral("%1 (%2)").arg(targetAcronym, outRuleId)
          : QStringLiteral("%1 (%2)").arg(label, outRuleId));
      checkbox->setEnabled(!outRuleId.isEmpty());
      return;
    }

    checkbox->setText(QStringLiteral("%1 rule (not available yet)").arg(targetAcronym));
    checkbox->setEnabled(false);
  };

  {
    const QSignalBlocker depareBlocker(m_ruleDepareCheck);
    const QSignalBlocker wrecksBlocker(m_ruleWrecksCheck);
    assignRule(QStringLiteral("DEPARE"), m_ruleDepareId, m_ruleDepareCheck);
    assignRule(QStringLiteral("WRECKS"), m_ruleWrecksId, m_ruleWrecksCheck);
  }

  std::uint32_t filterCount = 0;
  std::vector<chart_view_s52_rule_filter_t> filters;
  if(m_chartWidget->s52RuleFilters(nullptr, &filterCount) == chart_view_status_ok && filterCount > 0U) {
    filters.resize(filterCount);
    if(m_chartWidget->s52RuleFilters(filters.data(), &filterCount) != chart_view_status_ok) {
      filters.clear();
    }
  }

  auto currentEnabled = [&](const QString &ruleId) {
    if(ruleId.isEmpty()) {
      return true;
    }

    for(const auto &filter : filters) {
      const QString candidate = QString::fromUtf8(filter.rule_id == nullptr ? "" : filter.rule_id);
      if(candidate == ruleId) {
        return filter.enabled != 0U;
      }
    }
    return true;
  };

  {
    const QSignalBlocker depareBlocker(m_ruleDepareCheck);
    const QSignalBlocker wrecksBlocker(m_ruleWrecksCheck);
    m_ruleDepareCheck->setChecked(currentEnabled(m_ruleDepareId));
    m_ruleWrecksCheck->setChecked(currentEnabled(m_ruleWrecksId));
  }
}

void MainWindow::applyPhase5MarinerSettings()
{
  if(m_chartWidget == nullptr || !ensureRuntimeInitialized()) {
    return;
  }

  chart_view_s52_mariner_settings_t settings{};
  if(m_chartWidget->s52MarinerSettings(&settings) != chart_view_status_ok) {
    return;
  }

  settings.palette = static_cast<chart_view_s52_color_palette_t>(m_paletteCombo->currentData().toInt());
  settings.display_category = static_cast<chart_view_s52_display_category_t>(m_displayCategoryCombo->currentData().toInt());
  settings.show_text = m_showTextCheck->isChecked() ? 1U : 0U;
  settings.show_soundings = m_showSoundingsCheck->isChecked() ? 1U : 0U;
  settings.simplified_points = m_simplifiedPointsCheck->isChecked() ? 1U : 0U;
  settings.two_shades = m_twoShadesCheck->isChecked() ? 1U : 0U;
  settings.shallow_pattern = m_shallowPatternCheck->isChecked() ? 1U : 0U;
  settings.full_sector_lights = m_fullSectorLightsCheck->isChecked() ? 1U : 0U;
  settings.symbolized_boundaries = m_symbolizedBoundariesCheck->isChecked() ? 1U : 0U;
  settings.honor_scamin = m_honorScaminCheck->isChecked() ? 1U : 0U;

  if(m_chartWidget->setS52MarinerSettings(settings) == chart_view_status_ok) {
    statusBar()->showMessage(
      QStringLiteral("Applied Phase 5 settings: %1 / %2")
        .arg(paletteName(settings.palette), displayCategoryName(settings.display_category)),
      3000);
  }
}

void MainWindow::applyPhase5ClassFilters()
{
  if(m_chartWidget == nullptr || !ensureRuntimeInitialized()) {
    return;
  }

  constexpr std::array<const char *, 3> kNames{"DEPARE", "LIGHTS", "WRECKS"};
  const std::array<QCheckBox *, 3> checks{
    m_classDepareCheck,
    m_classLightsCheck,
    m_classWrecksCheck};

  std::array<chart_view_s57_class_filter_t, 3> filters{};
  for(std::size_t index = 0; index < filters.size(); ++index) {
    filters[index].object_acronym = kNames[index];
    filters[index].enabled = checks[index]->isChecked() ? 1U : 0U;
  }

  if(m_chartWidget->setS57ClassFilters(filters.data(), static_cast<std::uint32_t>(filters.size()))
     == chart_view_status_ok) {
    statusBar()->showMessage(QStringLiteral("Applied S57 class filters"), 3000);
  }
}

void MainWindow::applyPhase5RuleFilters()
{
  if(m_chartWidget == nullptr || !ensureRuntimeInitialized()) {
    return;
  }

  std::vector<QByteArray> ruleIds;
  std::vector<chart_view_s52_rule_filter_t> filters;

  auto appendRule = [&](const QString &ruleId, bool enabled) {
    if(ruleId.isEmpty()) {
      return;
    }

    ruleIds.push_back(ruleId.toUtf8());
    filters.push_back({ruleIds.back().constData(), enabled ? 1U : 0U});
  };

  appendRule(m_ruleDepareId, m_ruleDepareCheck->isChecked());
  appendRule(m_ruleWrecksId, m_ruleWrecksCheck->isChecked());

  if(m_chartWidget->setS52RuleFilters(filters.data(), static_cast<std::uint32_t>(filters.size()))
     == chart_view_status_ok) {
    statusBar()->showMessage(QStringLiteral("Applied selected S-52 rule filters"), 3000);
  }
}

bool MainWindow::applyPhase5DemoControlPreset()
{
  if(m_chartWidget == nullptr || !ensureRuntimeInitialized()) {
    return false;
  }

  refreshRuleFilterControls();

  {
    const QSignalBlocker paletteBlocker(m_paletteCombo);
    const QSignalBlocker categoryBlocker(m_displayCategoryCombo);
    const QSignalBlocker textBlocker(m_showTextCheck);
    const QSignalBlocker soundingBlocker(m_showSoundingsCheck);
    const QSignalBlocker simplifiedBlocker(m_simplifiedPointsCheck);
    const QSignalBlocker twoShadesBlocker(m_twoShadesCheck);
    const QSignalBlocker shallowPatternBlocker(m_shallowPatternCheck);
    const QSignalBlocker fullSectorBlocker(m_fullSectorLightsCheck);
    const QSignalBlocker boundariesBlocker(m_symbolizedBoundariesCheck);
    const QSignalBlocker scaminBlocker(m_honorScaminCheck);
    const QSignalBlocker depareBlocker(m_classDepareCheck);
    const QSignalBlocker lightsBlocker(m_classLightsCheck);
    const QSignalBlocker wrecksBlocker(m_classWrecksCheck);
    const QSignalBlocker depareRuleBlocker(m_ruleDepareCheck);
    const QSignalBlocker wrecksRuleBlocker(m_ruleWrecksCheck);

    m_paletteCombo->setCurrentIndex(m_paletteCombo->findData(static_cast<int>(chart_view_s52_palette_night)));
    m_displayCategoryCombo->setCurrentIndex(
      m_displayCategoryCombo->findData(static_cast<int>(chart_view_s52_display_all)));
    m_showTextCheck->setChecked(false);
    m_showSoundingsCheck->setChecked(false);
    m_simplifiedPointsCheck->setChecked(true);
    m_twoShadesCheck->setChecked(true);
    m_shallowPatternCheck->setChecked(true);
    m_fullSectorLightsCheck->setChecked(true);
    m_symbolizedBoundariesCheck->setChecked(true);
    m_honorScaminCheck->setChecked(true);
    m_classDepareCheck->setChecked(false);
    m_classLightsCheck->setChecked(true);
    m_classWrecksCheck->setChecked(true);
    if(m_ruleDepareCheck->isEnabled()) {
      m_ruleDepareCheck->setChecked(true);
    }
    if(m_ruleWrecksCheck->isEnabled()) {
      m_ruleWrecksCheck->setChecked(false);
    }
  }

  applyPhase5MarinerSettings();
  applyPhase5ClassFilters();
  applyPhase5RuleFilters();

  chart_view_s52_mariner_settings_t settings{};
  if(m_chartWidget->s52MarinerSettings(&settings) != chart_view_status_ok) {
    return false;
  }

  return settings.palette == chart_view_s52_palette_night
      && settings.display_category == chart_view_s52_display_all
      && settings.show_text == 0U
      && settings.show_soundings == 0U
      && settings.simplified_points != 0U;
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

void MainWindow::openChartDirectoryDialog()
{
  const auto path = QFileDialog::getExistingDirectory(
    this,
    QStringLiteral("Open Chart Directory"),
    QString());

  if(path.isEmpty()) {
    return;
  }

  if(!openChartDirectory(path)) {
    QMessageBox::warning(
      this,
      QStringLiteral("Open Chart Directory"),
      QStringLiteral("Failed to open chart directory:\n%1").arg(path));
  }
}

}// namespace chart_standalone
