#include <chart_view/qtwidgets/chart_view_widget.hpp>

#include "runtime_bridge.hpp"

#include <QImage>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QStringList>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <memory>

namespace chart_view::qtwidgets {

class ChartViewWidget::Impl
{
public:
  RuntimeBridge bridge;
  QLabel *status_label = nullptr;
  chart_view_render_frame_result_t lastResult{};
  QImage frameImage;
};

namespace {
QString chartScaffoldingSummary(std::uint32_t feature_flags)
{
  QStringList enabled_scaffolding;

  if((feature_flags & CHART_VIEW_FEATURE_S57) != 0U) {
    enabled_scaffolding.append(QStringLiteral("S-57"));
  }

  if((feature_flags & CHART_VIEW_FEATURE_CM93) != 0U) {
    enabled_scaffolding.append(QStringLiteral("CM93"));
  }

  if((feature_flags & CHART_VIEW_FEATURE_S101) != 0U) {
    enabled_scaffolding.append(QStringLiteral("S-101"));
  }

  if(enabled_scaffolding.isEmpty()) {
    return QStringLiteral("none");
  }

  return enabled_scaffolding.join(QStringLiteral(", "));
}

void updateStatusLabel(QLabel *label, const RuntimeBridge &bridge)
{
  if(label == nullptr) {
    return;
  }

  if(!bridge.hasRuntime()) {
    label->setText(QStringLiteral("No runtime attached."));
    return;
  }

  chart_view_runtime_info_t info{};
  const auto status = bridge.queryInfo(&info);
  if(status != chart_view_status_ok) {
    label->setText(QStringLiteral("chart_runtime info query failed (%1).").arg(static_cast<int>(status)));
    return;
  }

  const auto backend_name = (info.enabled_feature_flags & CHART_VIEW_FEATURE_QT_RHI) != 0U
                              ? QStringLiteral("Qt 6 RHI")
                              : QStringLiteral("disabled");

  label->setText(
    QStringLiteral("%1 %2 ready. Backend policy: %3. Chart scaffolding: %4.")
      .arg(QString::fromUtf8(info.project_name))
      .arg(QString::fromUtf8(info.project_version))
      .arg(backend_name)
      .arg(chartScaffoldingSummary(info.enabled_feature_flags)));
}

constexpr double kMetresPerDegLat = 111320.0;
constexpr double kPixelsPerMetre = 3779.5275591;

double metresPerDegreeLon(double centerLat) noexcept
{
  const auto cosLat = std::cos(centerLat * 3.14159265358979323846 / 180.0);
  return kMetresPerDegLat * (cosLat > 1e-6 ? cosLat : 1e-6);
}

double degreesPerPixelLon(const chart_view_viewport_t &vp) noexcept
{
  return vp.scale_denominator / (kPixelsPerMetre * metresPerDegreeLon(vp.center_lat));
}

double degreesPerPixelLat(const chart_view_viewport_t &vp) noexcept
{
  return vp.scale_denominator / (kPixelsPerMetre * kMetresPerDegLat);
}

void applyAnchorZoom(chart_view_viewport_t &newViewport,
                     const chart_view_viewport_t &oldViewport,
                     const QPointF &anchorPixel) noexcept
{
  if(oldViewport.pixel_width <= 0 || oldViewport.pixel_height <= 0 ||
     newViewport.pixel_width <= 0 || newViewport.pixel_height <= 0) {
    return;
  }

  const auto dxOld = anchorPixel.x() - (static_cast<double>(oldViewport.pixel_width) * 0.5);
  const auto dyOld = anchorPixel.y() - (static_cast<double>(oldViewport.pixel_height) * 0.5);
  const auto worldLon = oldViewport.center_lon + (dxOld * degreesPerPixelLon(oldViewport));
  const auto worldLat = oldViewport.center_lat - (dyOld * degreesPerPixelLat(oldViewport));

  const auto dxNew = anchorPixel.x() - (static_cast<double>(newViewport.pixel_width) * 0.5);
  const auto dyNew = anchorPixel.y() - (static_cast<double>(newViewport.pixel_height) * 0.5);
  newViewport.center_lon = worldLon - (dxNew * degreesPerPixelLon(newViewport));
  newViewport.center_lat = worldLat + (dyNew * degreesPerPixelLat(newViewport));
}
}// namespace

ChartViewWidget::ChartViewWidget(QWidget *parent)
  : QWidget(parent)
  , impl_(std::make_unique<Impl>())
{
  setObjectName(QStringLiteral("chartViewWidget"));

  auto *layout = new QVBoxLayout(this);
  auto *title_label = new QLabel(QStringLiteral("chart_qtwidgets host shell"), this);
  auto *status_label = new QLabel(this);

  title_label->setObjectName(QStringLiteral("chartViewWidgetTitle"));
  status_label->setObjectName(QStringLiteral("chartViewWidgetStatus"));

  layout->addWidget(title_label);
  layout->addWidget(status_label);
  layout->addStretch(1);

  impl_->status_label = status_label;

  // By default, create and own an internal runtime.
  const auto create_status = impl_->bridge.createOwnedRuntime();
  if(create_status != chart_view_status_ok) {
    impl_->status_label->setText(
      QStringLiteral("chart_runtime bootstrap failed (%1).").arg(static_cast<int>(create_status)));
    return;
  }

  updateStatusLabel(impl_->status_label, impl_->bridge);
}

ChartViewWidget::~ChartViewWidget() = default;

bool ChartViewWidget::attachRuntime(chart_view_runtime_t *runtime)
{
  if(runtime == nullptr) {
    return false;
  }

  // Detach whatever is currently connected (owned or external).
  impl_->bridge.detach();

  if(!impl_->bridge.attachExternal(runtime)) {
    updateStatusLabel(impl_->status_label, impl_->bridge);
    return false;
  }

  updateStatusLabel(impl_->status_label, impl_->bridge);
  return true;
}

void ChartViewWidget::detachRuntime()
{
  impl_->bridge.detach();
  updateStatusLabel(impl_->status_label, impl_->bridge);
}

bool ChartViewWidget::hasRuntime() const noexcept { return impl_->bridge.hasRuntime(); }

chart_view_runtime_t *ChartViewWidget::runtimeHandle() const noexcept { return impl_->bridge.handle(); }

chart_view_runtime_info_t ChartViewWidget::runtimeInfo() const noexcept
{
  chart_view_runtime_info_t info{};
  (void)impl_->bridge.queryInfo(&info);
  return info;
}

QString ChartViewWidget::statusText() const
{
  if(impl_->status_label == nullptr) {
    return {};
  }

  return impl_->status_label->text();
}

chart_view_status_t ChartViewWidget::loadSenc(const void *sencData, std::uint32_t size)
{
  return impl_->bridge.loadSenc(sencData, size);
}

chart_view_render_frame_result_t ChartViewWidget::lastRenderResult() const noexcept
{
  return impl_->lastResult;
}

void ChartViewWidget::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);

  if(!impl_->bridge.hasRuntime()) {
    return;
  }

  chart_view_viewport_t vp{};
  const auto queryStatus = impl_->bridge.queryViewport(&vp);
  if(queryStatus != chart_view_status_ok) {
    return;
  }

  if(vp.scale_denominator <= 0.0) {
    vp.scale_denominator = 100000.0;
  }

  vp.pixel_width = std::max(event->size().width(), 1);
  vp.pixel_height = std::max(event->size().height(), 1);

  (void)impl_->bridge.setViewport(vp);
  update();
}

void ChartViewWidget::paintEvent(QPaintEvent *event)
{
  QWidget::paintEvent(event);

  if(!impl_->bridge.hasRuntime()) {
    return;
  }

  if(impl_->bridge.renderFrame(&impl_->lastResult) != chart_view_status_ok) {
    return;
  }

  chart_view_frame_buffer_info_t frameInfo{};
  if(impl_->bridge.queryFrameBufferInfo(&frameInfo) != chart_view_status_ok
     || frameInfo.pixel_width == 0U
     || frameInfo.pixel_height == 0U
     || frameInfo.rgba_size_bytes == 0U) {
    return;
  }

  if(impl_->frameImage.size() != QSize(static_cast<int>(frameInfo.pixel_width),
                                       static_cast<int>(frameInfo.pixel_height))
     || impl_->frameImage.format() != QImage::Format_RGBA8888) {
    impl_->frameImage = QImage(
      static_cast<int>(frameInfo.pixel_width),
      static_cast<int>(frameInfo.pixel_height),
      QImage::Format_RGBA8888);
  }

  if(impl_->frameImage.isNull()) {
    return;
  }

  const auto copyStatus = impl_->bridge.copyFrameRgba(
    impl_->frameImage.bits(),
    static_cast<std::uint32_t>(impl_->frameImage.sizeInBytes()));
  if(copyStatus != chart_view_status_ok) {
    return;
  }

  QPainter painter(this);
  painter.drawImage(rect(), impl_->frameImage);
}

void ChartViewWidget::wheelEvent(QWheelEvent *event)
{
  if(!impl_->bridge.hasRuntime()) {
    QWidget::wheelEvent(event);
    return;
  }

  const auto deltaY = event->angleDelta().y();
  if(deltaY == 0) {
    QWidget::wheelEvent(event);
    return;
  }

  chart_view_viewport_t oldViewport{};
  if(impl_->bridge.queryViewport(&oldViewport) != chart_view_status_ok || oldViewport.scale_denominator <= 0.0) {
    QWidget::wheelEvent(event);
    return;
  }

  const auto steps = std::max(1, std::abs(deltaY) / 120);
  chart_view_zoom_result_t zoomResult{};
  const auto zoomStatus =
    impl_->bridge.stepZoom(deltaY > 0 ? steps : -steps, &zoomResult);
  if(zoomStatus != chart_view_status_ok) {
    QWidget::wheelEvent(event);
    return;
  }

  auto anchoredViewport = zoomResult.viewport;
  applyAnchorZoom(anchoredViewport, oldViewport, event->position());
  if(impl_->bridge.setViewport(anchoredViewport) != chart_view_status_ok) {
    QWidget::wheelEvent(event);
    return;
  }

  update();
  event->accept();
}

}// namespace chart_view::qtwidgets
