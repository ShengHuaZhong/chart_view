#ifndef CHART_VIEW_RUNTIME_POINT_SYMBOL_RENDERER_HPP
#define CHART_VIEW_RUNTIME_POINT_SYMBOL_RENDERER_HPP

#include "portrayal/portrayal_registry.hpp"
#include "rhi_render_backend.hpp"

#include <string_view>

namespace chart_view::runtime {

class PointSymbolRenderer
{
public:
  PointSymbolRenderer() = default;

  [[nodiscard]] bool render(
    std::string_view styleKey,
    SurfacePoint anchor,
    const portrayal::SymbolRule &rule,
    RhiRenderBackend &backend) const;
};

}// namespace chart_view::runtime

#endif
