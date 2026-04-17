#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S52_SOURCE_CATALOG_COMPILER_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S52_SOURCE_CATALOG_COMPILER_HPP

#include "s52_compiled_catalog.hpp"
#include "opencpn_s52_resource_bundle.hpp"
#include "s52_source_catalog.hpp"

#include <string>
#include <string_view>

namespace chart_view::runtime::portrayal {

class S52SourceCatalogCompiler
{
public:
  [[nodiscard]] static S52CompiledCatalog compile(const S52SourceCatalog &sourceCatalog,
                                                  std::string_view catalogIdHint = {});
  [[nodiscard]] static S52CompiledCatalog compileBuiltin();
  [[nodiscard]] static S52CompiledCatalog compileOpenCpnBundle(const OpenCpnS52ResourceBundle &bundle,
                                                               std::string *error = nullptr);
  [[nodiscard]] static S52CompiledCatalog compilePreferred();
};

} // namespace chart_view::runtime::portrayal

#endif
