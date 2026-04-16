#include "coverage_index.hpp"

#include "../projection/projected_bounds.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_set>

namespace chart_view::runtime::catalog {

namespace {

constexpr double kMinCellSize = 1e-6;

[[nodiscard]] projection::ProjectedExtent invalidProjectedExtent() noexcept
{
  return {1.0, 1.0, 0.0, 0.0};
}

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
  m_projectedExtents.assign(m_entries.size(), invalidProjectedExtent());

  const auto projectionContext = projection::ProjectionContext::createMercator();
  if(!projectionContext.isValid()) {
    clear();
    m_lastError = projectionContext.lastError();
    return false;
  }

  bool haveBounds = false;
  for(std::size_t i = 0; i < m_entries.size(); ++i) {
    const auto &entry = m_entries[i];
    if(!entry.extent.isValid()) {
      continue;
    }

    projection::ProjectedExtent projectedExtent{};
    if(!projectionContext.projectExtent(entry.extent, projectedExtent)) {
      continue;
    }

    m_projectedExtents[i] = projectedExtent;

    if(!haveBounds) {
      m_bounds = projectedExtent;
      haveBounds = true;
    } else {
      m_bounds.minX = std::min(m_bounds.minX, projectedExtent.minX);
      m_bounds.minY = std::min(m_bounds.minY, projectedExtent.minY);
      m_bounds.maxX = std::max(m_bounds.maxX, projectedExtent.maxX);
      m_bounds.maxY = std::max(m_bounds.maxY, projectedExtent.maxY);
    }
  }

  if(!haveBounds) {
    return true;
  }

  m_gridDimension = std::max(
    1,
    static_cast<int>(std::ceil(std::sqrt(static_cast<double>(m_entries.size())))));

  const double xSpan = std::max(m_bounds.maxX - m_bounds.minX, kMinCellSize);
  const double ySpan = std::max(m_bounds.maxY - m_bounds.minY, kMinCellSize);
  m_cellWidth = xSpan / static_cast<double>(m_gridDimension);
  m_cellHeight = ySpan / static_cast<double>(m_gridDimension);

  for(std::size_t i = 0; i < m_entries.size(); ++i) {
    const auto &extent = m_projectedExtents[i];
    if(!extent.isValid()) {
      continue;
    }

    const auto minCell = makeCellKey(extent.minX, extent.minY);
    const auto maxCell = makeCellKey(extent.maxX, extent.maxY);

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
  m_projectedExtents.clear();
  m_buckets.clear();
  m_bounds = invalidProjectedExtent();
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

  const auto projectionContext = projection::ProjectionContext::createMercator();
  if(!projectionContext.isValid()) {
    return results;
  }

  projection::ProjectedExtent projectedViewport{};
  if(!projectionContext.projectExtent(viewportExtent, projectedViewport)) {
    return results;
  }

  const auto minCell = makeCellKey(projectedViewport.minX, projectedViewport.minY);
  const auto maxCell = makeCellKey(projectedViewport.maxX, projectedViewport.maxY);

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
        if(extentsOverlap(m_projectedExtents[entryIndex], projectedViewport)) {
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

  const auto cellX = static_cast<int>(std::floor((lon - m_bounds.minX) / m_cellWidth));
  const auto cellY = static_cast<int>(std::floor((lat - m_bounds.minY) / m_cellHeight));

  return {
    std::clamp(cellX, 0, m_gridDimension - 1),
    std::clamp(cellY, 0, m_gridDimension - 1)};
}

bool CoverageIndex::extentsOverlap(
  const projection::ProjectedExtent &lhs,
  const projection::ProjectedExtent &rhs) const noexcept
{
  return projection::projectedExtentsOverlap(lhs, rhs);
}

}// namespace chart_view::runtime::catalog
