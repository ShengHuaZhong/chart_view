#ifndef CHART_VIEW_RUNTIME_S57_S57_UPDATE_APPLICATION_HPP
#define CHART_VIEW_RUNTIME_S57_S57_UPDATE_APPLICATION_HPP

#include "s57_source_model.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace chart_view::runtime::s57 {

enum class S57UpdateOperation : std::uint8_t
{
  kBase = 0,
  kInsert = 1,
  kDelete = 2,
  kModify = 3
};

struct S57UpdateApplicationResult
{
  bool ok{false};
  std::string error;
  S57SourceModel model;
  std::vector<std::uint32_t> appliedUpdates;
};

[[nodiscard]] S57UpdateOperation decodeUpdateOperation(std::uint8_t raw) noexcept;

[[nodiscard]] S57UpdateApplicationResult applySequentialUpdates(
  S57SourceModel baseModel,
  const std::vector<S57SourceModel> &updates);

}// namespace chart_view::runtime::s57

#endif
