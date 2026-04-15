#include <chart_view/runtime/chart_runtime.h>

#if defined(CHART_VIEW_PACKAGE_HAS_QTWIDGETS)
#include <chart_view/qtwidgets/chart_view_widget.hpp>
#endif

int main()
{
  return chart_view_runtime_abi_version() == CHART_VIEW_RUNTIME_ABI_VERSION ? 0 : 1;
}
