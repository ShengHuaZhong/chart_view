#pragma once

#if defined(_WIN32) && defined(CHART_QTWIDGETS_BUILD)
  #define CHART_QTWIDGETS_API __declspec(dllexport)
#elif defined(_WIN32)
  #define CHART_QTWIDGETS_API __declspec(dllimport)
#else
  #define CHART_QTWIDGETS_API __attribute__((visibility("default")))
#endif

#include <memory>

#include <QWidget>

namespace chartsys::qtwidgets {

class RuntimeBridge;

class CHART_QTWIDGETS_API ChartViewWidget final : public QWidget {
public:
  explicit ChartViewWidget(QWidget* parent = nullptr);
  ~ChartViewWidget() override;

protected:
  auto resizeEvent(QResizeEvent* event) -> void override;
  auto paintEvent(QPaintEvent* event) -> void override;

private:
  std::unique_ptr<RuntimeBridge> bridge_;
};

} // namespace chartsys::qtwidgets
