#ifndef CHART_VIEW_RUNTIME_RENDER_TYPES_HPP
#define CHART_VIEW_RUNTIME_RENDER_TYPES_HPP

#include <array>
#include <cstdint>

namespace chart_view::runtime {

struct SurfacePoint
{
  int x{0};
  int y{0};
};

using SurfaceColor = std::array<std::uint8_t, 4>;

}// namespace chart_view::runtime

#endif
