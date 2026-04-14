#include "runtime/chart_runtime_c_api.h"
#include "runtime/chart_runtime_types.h"

#include <cstdlib>

auto main() -> int {
  constexpr uint32_t kViewportWidth = 800U;
  constexpr uint32_t kViewportHeight = 600U;

  const chart_runtime_create_info create_info{};

  chart_runtime_handle runtime = chart_runtime_create(&create_info);
  if(runtime == nullptr) {
    return EXIT_FAILURE;
  }

  if(chart_runtime_initialize(runtime) != 0) {
    chart_runtime_destroy(runtime);
    return EXIT_FAILURE;
  }

  const chart_runtime_viewport viewport{kViewportWidth, kViewportHeight, 1.0F};
  if(chart_runtime_set_viewport(runtime, &viewport) != 0) {
    chart_runtime_destroy(runtime);
    return EXIT_FAILURE;
  }

  if(chart_runtime_shutdown(runtime) != 0) {
    chart_runtime_destroy(runtime);
    return EXIT_FAILURE;
  }

  chart_runtime_destroy(runtime);
  return EXIT_SUCCESS;
}
