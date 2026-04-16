#ifndef CHART_VIEW_RUNTIME_AREA_SYMBOL_RENDERER_HPP
#define CHART_VIEW_RUNTIME_AREA_SYMBOL_RENDERER_HPP

#include "portrayal/portrayal_registry.hpp"
#include "rhi_render_backend.hpp"

#include <span>
#include <string_view>
#include <vector>

namespace chart_view::runtime {

class AreaSymbolRenderer
{
public:
  AreaSymbolRenderer() = default;

  [[nodiscard]] bool render(
    std::string_view assetId,
    std::string_view styleKey,
    std::span<const SurfacePoint> exterior,
    const std::vector<std::vector<SurfacePoint>> &holes,
    const portrayal::AreaFillRule &rule,
    RhiRenderBackend &backend) const;
};

}// namespace chart_view::runtime

#endif
