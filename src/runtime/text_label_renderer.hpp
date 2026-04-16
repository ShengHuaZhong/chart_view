#ifndef CHART_VIEW_RUNTIME_TEXT_LABEL_RENDERER_HPP
#define CHART_VIEW_RUNTIME_TEXT_LABEL_RENDERER_HPP

#include "chart_data/feature.hpp"
#include "portrayal/portrayal_registry.hpp"
#include "rhi_render_backend.hpp"
#include "unicode_text.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace chart_view::runtime {

struct LabelItem
{
  std::string text;
  std::u32string glyphText;
  SurfacePoint origin;
  SurfaceColor color{24U, 38U, 55U, 255U};
  std::uint32_t pixelSize{12U};
  int width{0};
  int height{0};
};

class TextLabelRenderer
{
public:
  TextLabelRenderer() = default;

  [[nodiscard]] std::optional<LabelItem> layout(
    std::string_view textKey,
    const chart_data::Feature &feature,
    SurfacePoint anchor,
    const portrayal::TextRule &rule) const;

  void render(const LabelItem &label, RhiRenderBackend &backend) const;
};

}// namespace chart_view::runtime

#endif
