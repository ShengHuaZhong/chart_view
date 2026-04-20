#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_E400_DAI_SOURCE_CATALOG_BRIDGE_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_E400_DAI_SOURCE_CATALOG_BRIDGE_HPP

#include "s52_source_catalog.hpp"

#include <filesystem>
#include <string>

namespace chart_view::runtime::portrayal {

struct E400DaiSourceCatalogParseResult
{
  bool ok{false};
  S52SourceCatalog catalog;
  std::string error;
};

class E400DaiSourceCatalogBridge
{
public:
  [[nodiscard]] static E400DaiSourceCatalogParseResult parseFile(const std::filesystem::path &path);
};

} // namespace chart_view::runtime::portrayal

#endif
