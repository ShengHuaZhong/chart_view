#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_OPENCPN_S52_RESOURCE_BUNDLE_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_OPENCPN_S52_RESOURCE_BUNDLE_HPP

#include <filesystem>
#include <string_view>

namespace chart_view::runtime::portrayal {

struct OpenCpnS52ResourceBundle
{
  std::filesystem::path rootDirectory;

  [[nodiscard]] std::filesystem::path chartsymbolsXmlPath() const
  {
    return rootDirectory / "chartsymbols.xml";
  }

  [[nodiscard]] std::filesystem::path resolve(std::string_view relativePath) const
  {
    return rootDirectory / std::filesystem::path(relativePath);
  }
};

} // namespace chart_view::runtime::portrayal

#endif
