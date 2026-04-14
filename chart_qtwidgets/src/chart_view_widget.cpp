#include "chart_qtwidgets/chart_view_widget.hpp"

#include "chart_qtwidgets/runtime_bridge.hpp"

#include <QPainter>
#include <QResizeEvent>

namespace chartsys::qtwidgets {

ChartViewWidget::ChartViewWidget(QWidget* parent) : QWidget(parent), bridge_(std::make_unique<RuntimeBridge>()) {
  setAttribute(Qt::WA_OpaquePaintEvent);
}

ChartViewWidget::~ChartViewWidget() = default;

auto ChartViewWidget::resizeEvent(QResizeEvent* event) -> void {
  QWidget::resizeEvent(event);
  if(bridge_) {
    bridge_->setViewport(width(), height(), devicePixelRatioF());
  }
}

auto ChartViewWidget::paintEvent(QPaintEvent* event) -> void {
  QWidget::paintEvent(event);

  QPainter painter(this);
  painter.fillRect(rect(), QColor(14, 31, 49));
}

} // namespace chartsys::qtwidgets
