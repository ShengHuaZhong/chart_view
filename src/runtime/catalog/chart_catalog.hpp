#ifndef CHART_VIEW_RUNTIME_CATALOG_CHART_CATALOG_HPP
#define CHART_VIEW_RUNTIME_CATALOG_CHART_CATALOG_HPP

#include "../chart_data/geometry.hpp"

#include <chart_view/runtime/chart_runtime_types.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace chart_view::runtime::catalog {

struct ChartCatalogEntry
{
  std::string id;
  std::filesystem::path sencPath;
  chart_view_chart_source_type_t sourceType{chart_view_chart_source_unknown};
  double nativeScale{0.0};
  chart_data::Extent extent;
  std::uint32_t usageBand{0};
  std::uint32_t edition{0};
  std::uint32_t update{0};
};

class ChartCatalog
{
public:
  ChartCatalog() = default;

  [[nodiscard]] bool loadDirectory(const std::filesystem::path &directory);

  void clear() noexcept;

  [[nodiscard]] bool empty() const noexcept { return m_entries.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return m_entries.size(); }
  [[nodiscard]] const std::vector<ChartCatalogEntry> &entries() const noexcept { return m_entries; }
  [[nodiscard]] const std::string &lastError() const noexcept { return m_lastError; }
  [[nodiscard]] const ChartCatalogEntry *findById(std::string_view id) const noexcept;

private:
  std::vector<ChartCatalogEntry> m_entries;
  std::string m_lastError;
};

}// namespace chart_view::runtime::catalog

#endif
