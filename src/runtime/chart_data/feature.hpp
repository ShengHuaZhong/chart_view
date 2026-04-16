#ifndef CHART_VIEW_RUNTIME_CHART_DATA_FEATURE_HPP
#define CHART_VIEW_RUNTIME_CHART_DATA_FEATURE_HPP

#include "geometry.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace chart_view::runtime::chart_data {

// Attribute value -- supports common S-57/S-101 attribute value types.
using AttributeIntList = std::vector<std::int64_t>;
using AttributeDoubleList = std::vector<double>;
using AttributeStringList = std::vector<std::string>;
using AttributeValue = std::variant<
  std::int64_t,
  double,
  std::string,
  AttributeIntList,
  AttributeDoubleList,
  AttributeStringList>;

// A single chart feature (point, line, or area).
struct Feature
{
  // Unique ID within the dataset (e.g. S-57 FOID or record number).
  std::uint64_t id{0};

  // Object class code (e.g. S-57 OBJL value).
  std::uint32_t classCode{0};

  // Object class acronym (e.g. "DEPARE", "SOUNDG", "COALNE").
  std::string classAcronym;

  // Geometry -- exactly one of point/line/area.
  Geometry geometry;

  // Named attributes.
  std::unordered_map<std::string, AttributeValue> attributes;
};

}// namespace chart_view::runtime::chart_data

#endif
