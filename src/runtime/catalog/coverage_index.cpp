#include "coverage_index.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_set>

namespace chart_view::runtime::catalog {

namespace {

constexpr double kMinCellSize = 1e-6;

bool compareEntries(const ChartCatalogEntry *lhs, const ChartCatalogEntry *rhs)
{
  if(lhs->id != rhs->id) {
    return lhs->id < rhs->id;
  }
  return lhs->sencPath.generic_string() < rhs->sencPath.generic_string();
}

}// namespace

std::size_t CoverageIndex::CellKeyHash::operator()(const CellKey &key) const noexcept
{
  return (static_cast<std::size_t>(static_cast<std::uint32_t>(key.x)) << 32U) ^
         static_cast<std::size_t>(static_cast<std::uint32_t>(key.y));
}

bool CoverageIndex::build(const ChartCatalog &catalog)
{
  clear();

  if(catalog.empty()) {
    return true;
  }

  m_entries = catalog.entries();

  bool haveBounds = false;
  for(const auto &entry : m_entries) {
    if(!entry.extent.isValid()) {
      continue;
    }

    if(!haveBounds) {
      m_bounds = entry.extent;
      haveBounds = true;
    } else {
      m_bounds.minLon = std::min(m_bounds.minLon, entry.extent.minLon);
      m_bounds.minLat = std::min(m_bounds.minLat, entry.extent.minLat);
      m_bounds.maxLon = std::max(m_bounds.maxLon, entry.extent.maxLon);
      m_bounds.maxLat = std::max(m_bounds.maxLat, entry.extent.maxLat);
    }
  }

  if(!haveBounds) {
    return true;
  }

  m_gridDimension = std::max(
    1,
    static_cast<int>(std::ceil(std::sqrt(static_cast<double>(m_entries.size())))));

  const double lonSpan = std::max(m_bounds.maxLon - m_bounds.minLon, kMinCellSize);
  const double latSpan = std::max(m_bounds.maxLat - m_bounds.minLat, kMinCellSize);
  m_cellWidth = lonSpan / static_cast<double>(m_gridDimension);
  m_cellHeight = latSpan / static_cast<double>(m_gridDimension);

  for(std::size_t i = 0; i < m_entries.size(); ++i) {
    const auto &extent = m_entries[i].extent;
    if(!extent.isValid()) {
      continue;
    }

    const auto minCell = makeCellKey(extent.minLon, extent.minLat);
    const auto maxCell = makeCellKey(extent.maxLon, extent.maxLat);

    for(int y = minCell.y; y <= maxCell.y; ++y) {
      for(int x = minCell.x; x <= maxCell.x; ++x) {
        m_buckets[{x, y}].push_back(i);
      }
    }
  }

  return true;
}

void CoverageIndex::clear() noexcept
{
  m_entries.clear();
  m_buckets.clear();
  m_bounds = {};
  m_cellWidth = 0.0;
  m_cellHeight = 0.0;
  m_gridDimension = 0;
  m_lastError.clear();
}

std::vector<const ChartCatalogEntry *> CoverageIndex::query(
  const chart_data::Extent &viewportExtent) const
{
  std::vector<const ChartCatalogEntry *> results;

  if(!viewportExtent.isValid() || m_entries.empty() || !hasGrid()) {
    return results;
  }

  const auto minCell = makeCellKey(viewportExtent.minLon, viewportExtent.minLat);
  const auto maxCell = makeCellKey(viewportExtent.maxLon, viewportExtent.maxLat);

  std::unordered_set<std::size_t> seen;
  for(int y = minCell.y; y <= maxCell.y; ++y) {
    for(int x = minCell.x; x <= maxCell.x; ++x) {
      const auto bucketIt = m_buckets.find({x, y});
      if(bucketIt == m_buckets.end()) {
        continue;
      }

      for(const auto entryIndex : bucketIt->second) {
        if(!seen.insert(entryIndex).second) {
          continue;
        }

        const auto &entry = m_entries[entryIndex];
        if(extentsOverlap(entry.extent, viewportExtent)) {
          results.push_back(&entry);
        }
      }
    }
  }

  std::sort(results.begin(), results.end(), compareEntries);
  return results;
}

bool CoverageIndex::hasGrid() const noexcept
{
  return m_gridDimension > 0 && m_cellWidth > 0.0 && m_cellHeight > 0.0;
}

CoverageIndex::CellKey CoverageIndex::makeCellKey(double lon, double lat) const noexcept
{
  if(!hasGrid()) {
    return {};
  }

  const auto cellX = static_cast<int>(std::floor((lon - m_bounds.minLon) / m_cellWidth));
  const auto cellY = static_cast<int>(std::floor((lat - m_bounds.minLat) / m_cellHeight));

  return {
    std::clamp(cellX, 0, m_gridDimension - 1),
    std::clamp(cellY, 0, m_gridDimension - 1)};
}

bool CoverageIndex::extentsOverlap(
  const chart_data::Extent &lhs,
  const chart_data::Extent &rhs) const noexcept
{
  return lhs.minLon <= rhs.maxLon && lhs.maxLon >= rhs.minLon &&
         lhs.minLat <= rhs.maxLat && lhs.maxLat >= rhs.minLat;
}

}// namespace chart_view::runtime::catalog
