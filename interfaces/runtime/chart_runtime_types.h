#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chart_runtime_handle_t* chart_runtime_handle;

typedef struct chart_runtime_create_info_t {
  uint32_t api_version;
} chart_runtime_create_info;

typedef struct chart_runtime_viewport_t {
  uint32_t width;
  uint32_t height;
  float device_pixel_ratio;
} chart_runtime_viewport;

typedef struct chart_runtime_scene_context_t {
  uint64_t token;
} chart_runtime_scene_context;

typedef struct chart_runtime_render_context_t {
  uint64_t token;
} chart_runtime_render_context;

#ifdef __cplusplus
}
#endif
