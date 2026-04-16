#ifndef CHART_VIEW_QTWIDGETS_CHART_VIEW_WIDGET_HPP
#define CHART_VIEW_QTWIDGETS_CHART_VIEW_WIDGET_HPP

#include <chart_view/qtwidgets/chart_qtwidgets_export.h>
#include <chart_view/runtime/chart_runtime.h>

#include <QString>
#include <QWidget>
#include <QWheelEvent>

#include <cstdint>
#include <memory>

namespace chart_view::qtwidgets {

class CHART_QTWIDGETS_EXPORT ChartViewWidget final : public QWidget
{
public:
  explicit ChartViewWidget(QWidget *parent = nullptr);
  ~ChartViewWidget() override;

  ChartViewWidget(const ChartViewWidget &) = delete;
  ChartViewWidget &operator=(const ChartViewWidget &) = delete;
  ChartViewWidget(ChartViewWidget &&) = delete;
  ChartViewWidget &operator=(ChartViewWidget &&) = delete;

  // Attach an externally-owned runtime handle.
  // The widget does NOT take ownership -- caller must keep the handle alive.
  // Returns false if runtime is null or already attached.
  bool attachRuntime(chart_view_runtime_t *runtime);

  // Detach the current runtime (whether self-owned or external).
  void detachRuntime();

  // Load SENC data into the attached runtime.
  // Returns chart_view_status_ok on success.
  chart_view_status_t loadSenc(const void *sencData, std::uint32_t size);

  // Access the last render frame result.
  [[nodiscard]] chart_view_render_frame_result_t lastRenderResult() const noexcept;

  [[nodiscard]] bool hasRuntime() const noexcept;
  [[nodiscard]] chart_view_runtime_t *runtimeHandle() const noexcept;
  [[nodiscard]] chart_view_runtime_info_t runtimeInfo() const noexcept;
  [[nodiscard]] QString statusText() const;

protected:
  void resizeEvent(QResizeEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}// namespace chart_view::qtwidgets

#endif
