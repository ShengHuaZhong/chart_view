#ifndef CHART_VIEW_RUNTIME_CATALOG_COVERAGE_INDEX_HPP
#define CHART_VIEW_RUNTIME_CATALOG_COVERAGE_INDEX_HPP

#include "chart_catalog.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace chart_view::runtime::catalog {

class CoverageIndex
{
public:
  CoverageIndex() = default;

  [[nodiscard]] bool build(const ChartCatalog &catalog);
  void clear() noexcept;

  [[nodiscard]] std::vector<const ChartCatalogEntry *> query(
    const chart_data::Extent &viewportExtent) const;

  [[nodiscard]] std::size_t size() const noexcept { return m_entries.size(); }
  [[nodiscard]] std::size_t bucketCount() const noexcept { return m_buckets.size(); }
  [[nodiscard]] const std::string &lastError() const noexcept { return m_lastError; }

private:
  struct CellKey
  {
    int x{0};
    int y{0};

    [[nodiscard]] bool operator==(const CellKey &other) const noexcept
    {
      return x == other.x && y == other.y;
    }
  };

  struct CellKeyHash
  {
    [[nodiscard]] std::size_t operator()(const CellKey &key) const noexcept;
  };

  [[nodiscard]] bool hasGrid() const noexcept;
  [[nodiscard]] CellKey makeCellKey(double lon, double lat) const noexcept;
  [[nodiscard]] bool extentsOverlap(
    const chart_data::Extent &lhs,
    const chart_data::Extent &rhs) const noexcept;

  std::vector<ChartCatalogEntry> m_entries;
  std::unordered_map<CellKey, std::vector<std::size_t>, CellKeyHash> m_buckets;
  chart_data::Extent m_bounds;
  double m_cellWidth{0.0};
  double m_cellHeight{0.0};
  int m_gridDimension{0};
  std::string m_lastError;
};

}// namespace chart_view::runtime::catalog

#endif
