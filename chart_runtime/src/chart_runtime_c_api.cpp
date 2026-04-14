#include "runtime/chart_runtime_c_api.h"
#include "runtime/chart_runtime_types.h"

struct chart_runtime_handle_t {
  bool initialized{false};
  chart_runtime_viewport viewport{};
  chart_runtime_scene_context scene_context{0};
  chart_runtime_render_context render_context{0};
};

chart_runtime_handle chart_runtime_create(const chart_runtime_create_info* /*create_info*/) {
  return new chart_runtime_handle_t{};
}

int chart_runtime_initialize(chart_runtime_handle runtime) {
  if(runtime == nullptr) {
    return -1;
  }

  runtime->initialized = true;
  runtime->scene_context.token = 1;
  runtime->render_context.token = 1;
  return 0;
}

int chart_runtime_set_viewport(chart_runtime_handle runtime, const chart_runtime_viewport* viewport) {
  if(runtime == nullptr || viewport == nullptr) {
    return -1;
  }

  runtime->viewport = *viewport;
  return 0;
}

int chart_runtime_shutdown(chart_runtime_handle runtime) {
  if(runtime == nullptr) {
    return -1;
  }

  runtime->initialized = false;
  runtime->scene_context.token = 0;
  runtime->render_context.token = 0;
  return 0;
}

void chart_runtime_destroy(chart_runtime_handle runtime) {
  delete runtime;
}
