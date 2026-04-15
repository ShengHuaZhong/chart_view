#ifndef CHART_VIEW_RUNTIME_CHART_DATA_FEATURE_CHART_DATASET_HPP
#define CHART_VIEW_RUNTIME_CHART_DATA_FEATURE_CHART_DATASET_HPP

#include "dataset_meta.hpp"
#include "feature.hpp"

#include <cstddef>
#include <vector>

namespace chart_view::runtime::chart_data {

// Unified chart dataset -- holds metadata and a flat list of features.
// All three source formats (S-57, CM93, S-101) normalize into this model.
class FeatureChartDataset
{
public:
  FeatureChartDataset() = default;

  // -- metadata --
  void setMeta(DatasetMeta meta) { m_meta = std::move(meta); }
  [[nodiscard]] const DatasetMeta &meta() const noexcept { return m_meta; }

  // -- features --
  void addFeature(Feature f) { m_features.push_back(std::move(f)); }
  void reserveFeatures(std::size_t n) { m_features.reserve(n); }
  void clearFeatures() noexcept { m_features.clear(); }

  [[nodiscard]] const std::vector<Feature> &features() const noexcept { return m_features; }
  [[nodiscard]] std::vector<Feature> &features() noexcept { return m_features; }
  [[nodiscard]] std::size_t featureCount() const noexcept { return m_features.size(); }
  [[nodiscard]] bool empty() const noexcept { return m_features.empty(); }

private:
  DatasetMeta m_meta;
  std::vector<Feature> m_features;
};

}// namespace chart_view::runtime::chart_data

#endif
