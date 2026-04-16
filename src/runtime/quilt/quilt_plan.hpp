#ifndef CHART_VIEW_RUNTIME_QUILT_QUILT_PLAN_HPP
#define CHART_VIEW_RUNTIME_QUILT_QUILT_PLAN_HPP

#include "../catalog/chart_catalog.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace chart_view::runtime::quilt {

struct QuiltLayer
{
  std::string chartId;
  std::filesystem::path sencPath;
  chart_view_chart_source_type_t sourceType{chart_view_chart_source_unknown};
  double nativeScale{0.0};
  std::uint32_t usageBand{0};
  std::uint32_t drawOrder{0};
  chart_data::Extent fullExtent;
  chart_data::Extent visibleExtent;

  [[nodiscard]] static QuiltLayer fromCatalogEntry(
    const catalog::ChartCatalogEntry &entry,
    std::uint32_t order,
    chart_data::Extent visible = {1.0, 1.0, 0.0, 0.0}) noexcept
  {
    if(!visible.isValid()) {
      visible = entry.extent;
    }

    return {
      entry.id,
      entry.sencPath,
      entry.sourceType,
      entry.nativeScale,
      entry.usageBand,
      order,
      entry.extent,
      visible};
  }
};

struct QuiltSelectionResult
{
  double viewportScaleDenominator{0.0};
  std::size_t candidateCount{0};
  std::vector<std::string> orderedChartIds;
};

class QuiltPlan
{
public:
  QuiltPlan() = default;

  void clear() noexcept
  {
    m_viewportExtent = {};
    m_viewportScaleDenominator = 0.0;
    m_layers.clear();
    m_selectionResult = {};
  }

  void setViewport(chart_data::Extent viewportExtent, double viewportScaleDenominator) noexcept
  {
    m_viewportExtent = viewportExtent;
    m_viewportScaleDenominator = viewportScaleDenominator;
  }

  void setSelectionResult(QuiltSelectionResult selectionResult)
  {
    m_selectionResult = std::move(selectionResult);
  }

  void addLayer(QuiltLayer layer)
  {
    m_layers.push_back(std::move(layer));
  }

  [[nodiscard]] bool empty() const noexcept { return m_layers.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return m_layers.size(); }
  [[nodiscard]] const chart_data::Extent &viewportExtent() const noexcept { return m_viewportExtent; }
  [[nodiscard]] double viewportScaleDenominator() const noexcept { return m_viewportScaleDenominator; }
  [[nodiscard]] const std::vector<QuiltLayer> &layers() const noexcept { return m_layers; }
  [[nodiscard]] const QuiltSelectionResult &selectionResult() const noexcept { return m_selectionResult; }

private:
  chart_data::Extent m_viewportExtent;
  double m_viewportScaleDenominator{0.0};
  std::vector<QuiltLayer> m_layers;
  QuiltSelectionResult m_selectionResult;
};

}// namespace chart_view::runtime::quilt

#endif
