#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S52_SOURCE_CATALOG_COMPILER_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S52_SOURCE_CATALOG_COMPILER_HPP

#include "s52_compiled_catalog.hpp"
#include "s52_source_catalog.hpp"

namespace chart_view::runtime::portrayal {

class S52SourceCatalogCompiler
{
public:
  [[nodiscard]] static S52CompiledCatalog compile(const S52SourceCatalog &sourceCatalog);
  [[nodiscard]] static S52CompiledCatalog compileBuiltin();
};

} // namespace chart_view::runtime::portrayal

#endif
