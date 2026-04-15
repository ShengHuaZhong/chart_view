#ifndef CHART_VIEW_RUNTIME_RHI_RENDER_BACKEND_HPP
#define CHART_VIEW_RUNTIME_RHI_RENDER_BACKEND_HPP

#include <chart_view/runtime/chart_runtime_types.h>

#include <QtCore/qtconfigmacros.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

QT_BEGIN_NAMESPACE
class QRhi;
class QRhiTexture;
class QRhiTextureRenderTarget;
class QRhiRenderPassDescriptor;
class QRhiCommandBuffer;
QT_END_NAMESPACE

namespace chart_view::runtime {

struct SurfacePoint
{
  int x{0};
  int y{0};
};

using SurfaceColor = std::array<std::uint8_t, 4>;

// Minimal render backend.
// Keeps the existing QRhi bootstrap alive while also owning a presentable
// RGBA frame buffer that can be copied into the Qt host for Phase 1 display.
class RhiRenderBackend
{
public:
  RhiRenderBackend();
  ~RhiRenderBackend();

  RhiRenderBackend(const RhiRenderBackend &) = delete;
  RhiRenderBackend &operator=(const RhiRenderBackend &) = delete;
  RhiRenderBackend(RhiRenderBackend &&) = delete;
  RhiRenderBackend &operator=(RhiRenderBackend &&) = delete;

  // Create the QRhi instance and offscreen render target.
  // Uses the Null backend for headless / testing; will be extended
  // to select D3D11/D3D12/Vulkan/Metal based on platform in later tasks.
  [[nodiscard]] chart_view_status_t initialize(int width, int height);
  [[nodiscard]] chart_view_status_t ensureSurfaceSize(int width, int height);

  // Execute a single frame clear and reset the presentable RGBA surface.
  [[nodiscard]] chart_view_status_t renderClearFrame(float r, float g, float b, float a);

  void drawPoint(SurfacePoint point, int radius, SurfaceColor color);
  void drawLine(SurfacePoint start, SurfacePoint end, int thickness, SurfaceColor color);
  void fillPolygon(std::span<const SurfacePoint> points, SurfaceColor fillColor);
  void drawClosedPolyline(std::span<const SurfacePoint> points, int thickness, SurfaceColor color);

  [[nodiscard]] chart_view_status_t copyFrameRgba(std::span<std::uint8_t> dst) const;
  [[nodiscard]] bool isInitialized() const noexcept;
  [[nodiscard]] QRhi *rhi() const noexcept;
  [[nodiscard]] int surfaceWidth() const noexcept;
  [[nodiscard]] int surfaceHeight() const noexcept;
  [[nodiscard]] std::uint32_t strideBytes() const noexcept;
  [[nodiscard]] std::size_t frameByteSize() const noexcept;

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

}// namespace chart_view::runtime

#endif
