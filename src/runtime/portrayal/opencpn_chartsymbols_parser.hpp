#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_OPENCPN_CHARTSYMBOLS_PARSER_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_OPENCPN_CHARTSYMBOLS_PARSER_HPP

#include "opencpn_s52_resource_bundle.hpp"
#include "s52_source_catalog.hpp"

#include <string>

namespace chart_view::runtime::portrayal {

struct OpenCpnChartsymbolsParseResult
{
  bool ok{false};
  S52SourceCatalog catalog;
  std::string error;
};

class OpenCpnChartsymbolsParser
{
public:
  [[nodiscard]] static OpenCpnChartsymbolsParseResult parseBundle(const OpenCpnS52ResourceBundle &bundle);
};

} // namespace chart_view::runtime::portrayal

#endif
