#ifndef CHART_VIEW_RUNTIME_LINE_SYMBOL_RENDERER_HPP
#define CHART_VIEW_RUNTIME_LINE_SYMBOL_RENDERER_HPP

#include "portrayal/portrayal_registry.hpp"
#include "rhi_render_backend.hpp"

#include <span>
#include <string_view>

namespace chart_view::runtime {

class LineSymbolRenderer
{
public:
  LineSymbolRenderer() = default;

  [[nodiscard]] bool render(
    std::string_view assetId,
    std::string_view styleKey,
    std::span<const SurfacePoint> points,
    const portrayal::LineStyleRule &rule,
    RhiRenderBackend &backend) const;
};

}// namespace chart_view::runtime

#endif
