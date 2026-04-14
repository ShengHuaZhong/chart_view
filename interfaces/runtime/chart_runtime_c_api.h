#pragma once

#include "chart_runtime_types.h"

#if defined(_WIN32) && defined(CHART_RUNTIME_BUILD)
  #define CHART_RUNTIME_API __declspec(dllexport)
#elif defined(_WIN32)
  #define CHART_RUNTIME_API __declspec(dllimport)
#else
  #define CHART_RUNTIME_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

CHART_RUNTIME_API chart_runtime_handle chart_runtime_create(const chart_runtime_create_info* create_info);
CHART_RUNTIME_API int chart_runtime_initialize(chart_runtime_handle runtime);
CHART_RUNTIME_API int chart_runtime_set_viewport(chart_runtime_handle runtime, const chart_runtime_viewport* viewport);
CHART_RUNTIME_API int chart_runtime_shutdown(chart_runtime_handle runtime);
CHART_RUNTIME_API void chart_runtime_destroy(chart_runtime_handle runtime);

#ifdef __cplusplus
}
#endif
