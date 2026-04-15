#ifndef CHART_VIEW_RUNTIME_VIEWPORT_STATE_HPP
#define CHART_VIEW_RUNTIME_VIEWPORT_STATE_HPP

#include <chart_view/runtime/chart_runtime_types.h>

#include <cstdint>

namespace chart_view::runtime {

// Internal viewport state with dirty tracking.
// Wraps the public chart_view_viewport_t DTO.
class ViewportState
{
public:
  ViewportState() = default;

  void set(const chart_view_viewport_t &vp) noexcept
  {
    m_viewport = vp;
    ++m_revision;
  }

  [[nodiscard]] const chart_view_viewport_t &viewport() const noexcept { return m_viewport; }
  [[nodiscard]] std::uint64_t revision() const noexcept { return m_revision; }

  [[nodiscard]] bool isValid() const noexcept
  {
    return m_viewport.pixel_width > 0
        && m_viewport.pixel_height > 0
        && m_viewport.scale_denominator > 0.0;
  }

private:
  chart_view_viewport_t m_viewport{};
  std::uint64_t m_revision{0};
};

}// namespace chart_view::runtime

#endif
