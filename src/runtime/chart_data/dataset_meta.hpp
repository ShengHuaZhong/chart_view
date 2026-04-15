#ifndef CHART_VIEW_RUNTIME_CHART_DATA_DATASET_META_HPP
#define CHART_VIEW_RUNTIME_CHART_DATA_DATASET_META_HPP

#include "geometry.hpp"

#include <chart_view/runtime/chart_runtime_types.h>

#include <cstdint>
#include <string>

namespace chart_view::runtime::chart_data {

// Metadata describing a chart dataset.
struct DatasetMeta
{
  // Human-readable dataset name (e.g. filename or cell name).
  std::string name;

  // Source format.
  chart_view_chart_source_type_t sourceType{chart_view_chart_source_unknown};

  // Native scale denominator (e.g. 50000 for 1:50000).
  double nativeScale{0.0};

  // Geographic extent of the dataset.
  Extent extent;

  // Compilation scale or usage band (IHO usage band, 1-6).
  std::uint32_t usageBand{0};

  // Optional edition / update number.
  std::uint32_t edition{0};
  std::uint32_t update{0};
};

}// namespace chart_view::runtime::chart_data

#endif
