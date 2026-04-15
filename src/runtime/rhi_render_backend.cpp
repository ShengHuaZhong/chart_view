#include "rhi_render_backend.hpp"

#include <rhi/qrhi.h>

#include <QColor>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

namespace chart_view::runtime {

class RhiRenderBackend::Impl
{
public:
  std::unique_ptr<QRhi> rhi;
  std::unique_ptr<QRhiTexture> colorTarget;
  std::unique_ptr<QRhiTextureRenderTarget> renderTarget;
  std::unique_ptr<QRhiRenderPassDescriptor> rpDesc;
  int surfaceWidth{0};
  int surfaceHeight{0};
  std::vector<std::uint8_t> rgba;
};

namespace {
std::uint8_t toByte(float value) noexcept
{
  const auto clamped = std::clamp(value, 0.0F, 1.0F);
  return static_cast<std::uint8_t>(std::lround(clamped * 255.0F));
}

template<typename T>
bool createOffscreenTarget(T &impl, int width, int height)
{
  impl.rpDesc.reset();
  impl.renderTarget.reset();
  impl.colorTarget.reset();

  impl.colorTarget.reset(
    impl.rhi->newTexture(QRhiTexture::RGBA8, QSize(width, height), 1, QRhiTexture::RenderTarget));
  if(!impl.colorTarget || !impl.colorTarget->create()) {
    return false;
  }

  QRhiTextureRenderTargetDescription rtDesc(impl.colorTarget.get());
  impl.renderTarget.reset(impl.rhi->newTextureRenderTarget(rtDesc));
  if(!impl.renderTarget) {
    return false;
  }

  impl.rpDesc.reset(impl.renderTarget->newCompatibleRenderPassDescriptor());
  if(!impl.rpDesc) {
    return false;
  }

  impl.renderTarget->setRenderPassDescriptor(impl.rpDesc.get());
  return impl.renderTarget->create();
}

template<typename T>
void blendPixel(T &impl, int x, int y, SurfaceColor color)
{
  if(x < 0 || y < 0 || x >= impl.surfaceWidth || y >= impl.surfaceHeight || impl.rgba.empty()) {
    return;
  }

  const auto offset = static_cast<std::size_t>((y * impl.surfaceWidth + x) * 4);
  const float srcAlpha = static_cast<float>(color[3]) / 255.0F;
  const float invAlpha = 1.0F - srcAlpha;

  for(int i = 0; i < 3; ++i) {
    const float dst = static_cast<float>(impl.rgba[offset + i]);
    const float src = static_cast<float>(color[static_cast<std::size_t>(i)]);
    impl.rgba[offset + i] = static_cast<std::uint8_t>(std::lround(src * srcAlpha + dst * invAlpha));
  }

  impl.rgba[offset + 3] = 255U;
}

template<typename T>
void fillSpan(T &impl, int y, int x0, int x1, SurfaceColor color)
{
  if(y < 0 || y >= impl.surfaceHeight) {
    return;
  }

  const int start = std::max(0, std::min(x0, x1));
  const int end = std::min(impl.surfaceWidth - 1, std::max(x0, x1));
  for(int x = start; x <= end; ++x) {
    blendPixel(impl, x, y, color);
  }
}
}// namespace

RhiRenderBackend::RhiRenderBackend()
  : m_impl(std::make_unique<Impl>())
{
}

RhiRenderBackend::~RhiRenderBackend() = default;

chart_view_status_t RhiRenderBackend::initialize(int width, int height)
{
  if(m_impl->rhi) {
    return chart_view_status_already_initialized;
  }

  if(width <= 0 || height <= 0) {
    return chart_view_status_invalid_argument;
  }

  QRhiNullInitParams nullParams;
  m_impl->rhi.reset(QRhi::create(QRhi::Null, &nullParams));
  if(!m_impl->rhi) {
    return chart_view_status_allocation_failure;
  }

  m_impl->surfaceWidth = width;
  m_impl->surfaceHeight = height;
  m_impl->rgba.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U, 0U);

  if(!createOffscreenTarget(*m_impl, width, height)) {
    m_impl->rhi.reset();
    return chart_view_status_allocation_failure;
  }

  return renderClearFrame(0.9F, 0.9F, 0.85F, 1.0F);
}

chart_view_status_t RhiRenderBackend::ensureSurfaceSize(int width, int height)
{
  if(width <= 0 || height <= 0) {
    return chart_view_status_invalid_argument;
  }

  if(!m_impl->rhi) {
    return initialize(width, height);
  }

  if(m_impl->surfaceWidth == width && m_impl->surfaceHeight == height) {
    return chart_view_status_ok;
  }

  m_impl->surfaceWidth = width;
  m_impl->surfaceHeight = height;
  m_impl->rgba.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U, 0U);

  if(!createOffscreenTarget(*m_impl, width, height)) {
    return chart_view_status_allocation_failure;
  }

  return chart_view_status_ok;
}

chart_view_status_t RhiRenderBackend::renderClearFrame(float r, float g, float b, float a)
{
  if(!m_impl->rhi) {
    return chart_view_status_not_initialized;
  }

  const SurfaceColor clearColor{toByte(r), toByte(g), toByte(b), toByte(a)};
  for(std::size_t offset = 0; offset + 3 < m_impl->rgba.size(); offset += 4) {
    m_impl->rgba[offset + 0] = clearColor[0];
    m_impl->rgba[offset + 1] = clearColor[1];
    m_impl->rgba[offset + 2] = clearColor[2];
    m_impl->rgba[offset + 3] = clearColor[3];
  }

  QRhiCommandBuffer *cb = nullptr;
  auto frameResult = m_impl->rhi->beginOffscreenFrame(&cb);
  if(frameResult != QRhi::FrameOpSuccess) {
    return chart_view_status_io_error;
  }

  cb->beginPass(m_impl->renderTarget.get(), QColor::fromRgbF(r, g, b, a), {1.0F, 0});
  cb->endPass();

  m_impl->rhi->endOffscreenFrame();

  return chart_view_status_ok;
}

void RhiRenderBackend::drawPoint(SurfacePoint point, int radius, SurfaceColor color)
{
  if(radius < 0) {
    return;
  }

  for(int dy = -radius; dy <= radius; ++dy) {
    for(int dx = -radius; dx <= radius; ++dx) {
      if(dx * dx + dy * dy > radius * radius) {
        continue;
      }
      blendPixel(*m_impl, point.x + dx, point.y + dy, color);
    }
  }
}

void RhiRenderBackend::drawLine(SurfacePoint start, SurfacePoint end, int thickness, SurfaceColor color)
{
  int x0 = start.x;
  int y0 = start.y;
  const int x1 = end.x;
  const int y1 = end.y;

  const int dx = std::abs(x1 - x0);
  const int sx = x0 < x1 ? 1 : -1;
  const int dy = -std::abs(y1 - y0);
  const int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  const int radius = std::max(0, thickness / 2);

  while(true) {
    drawPoint({x0, y0}, radius, color);
    if(x0 == x1 && y0 == y1) {
      break;
    }
    const int e2 = err * 2;
    if(e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if(e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void RhiRenderBackend::fillPolygon(std::span<const SurfacePoint> points, SurfaceColor fillColor)
{
  if(points.size() < 3 || m_impl->surfaceWidth <= 0 || m_impl->surfaceHeight <= 0) {
    return;
  }

  int minY = std::numeric_limits<int>::max();
  int maxY = std::numeric_limits<int>::min();
  for(const auto &point : points) {
    minY = std::min(minY, point.y);
    maxY = std::max(maxY, point.y);
  }

  minY = std::max(minY, 0);
  maxY = std::min(maxY, m_impl->surfaceHeight - 1);

  std::vector<int> intersections;
  intersections.reserve(points.size());

  for(int y = minY; y <= maxY; ++y) {
    intersections.clear();

    for(std::size_t i = 0; i < points.size(); ++i) {
      const auto &a = points[i];
      const auto &b = points[(i + 1) % points.size()];
      if(a.y == b.y) {
        continue;
      }
      if((a.y <= y && b.y > y) || (b.y <= y && a.y > y)) {
        const double t = static_cast<double>(y - a.y) / static_cast<double>(b.y - a.y);
        const int x = static_cast<int>(std::lround(a.x + (b.x - a.x) * t));
        intersections.push_back(x);
      }
    }

    std::sort(intersections.begin(), intersections.end());
    for(std::size_t i = 0; i + 1 < intersections.size(); i += 2) {
      fillSpan(*m_impl, y, intersections[i], intersections[i + 1], fillColor);
    }
  }
}

void RhiRenderBackend::drawClosedPolyline(std::span<const SurfacePoint> points,
                                          int thickness,
                                          SurfaceColor color)
{
  if(points.size() < 2) {
    return;
  }

  for(std::size_t i = 0; i < points.size(); ++i) {
    const auto &start = points[i];
    const auto &end = points[(i + 1) % points.size()];
    drawLine(start, end, thickness, color);
  }
}

chart_view_status_t RhiRenderBackend::copyFrameRgba(std::span<std::uint8_t> dst) const
{
  if(!m_impl->rhi || m_impl->rgba.empty()) {
    return chart_view_status_not_initialized;
  }

  if(dst.size() != m_impl->rgba.size()) {
    return chart_view_status_invalid_argument;
  }

  std::memcpy(dst.data(), m_impl->rgba.data(), m_impl->rgba.size());
  return chart_view_status_ok;
}

bool RhiRenderBackend::isInitialized() const noexcept
{
  return m_impl->rhi != nullptr;
}

QRhi *RhiRenderBackend::rhi() const noexcept
{
  return m_impl->rhi.get();
}

int RhiRenderBackend::surfaceWidth() const noexcept
{
  return m_impl->surfaceWidth;
}

int RhiRenderBackend::surfaceHeight() const noexcept
{
  return m_impl->surfaceHeight;
}

std::uint32_t RhiRenderBackend::strideBytes() const noexcept
{
  return static_cast<std::uint32_t>(std::max(0, m_impl->surfaceWidth) * 4);
}

std::size_t RhiRenderBackend::frameByteSize() const noexcept
{
  return m_impl->rgba.size();
}

}// namespace chart_view::runtime
