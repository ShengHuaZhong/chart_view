#ifndef CHART_VIEW_RUNTIME_CHART_RUNTIME_C_API_H
#define CHART_VIEW_RUNTIME_CHART_RUNTIME_C_API_H

#include <chart_view/runtime/chart_runtime_export.h>
#include <chart_view/runtime/chart_runtime_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -- Version query ----------------------------------------------- */

CHART_RUNTIME_EXPORT uint32_t chart_view_runtime_abi_version(void);
CHART_RUNTIME_EXPORT const char *chart_view_runtime_version_string(void);

/* -- Lifecycle --------------------------------------------------- */

CHART_RUNTIME_EXPORT chart_view_status_t chart_view_runtime_create(chart_view_runtime_t **out_runtime);
CHART_RUNTIME_EXPORT chart_view_status_t chart_view_runtime_initialize(chart_view_runtime_t *runtime);
CHART_RUNTIME_EXPORT chart_view_status_t chart_view_runtime_shutdown(chart_view_runtime_t *runtime);
CHART_RUNTIME_EXPORT void chart_view_runtime_destroy(chart_view_runtime_t *runtime);

/* -- Info query -------------------------------------------------- */

CHART_RUNTIME_EXPORT chart_view_status_t
  chart_view_runtime_get_info(const chart_view_runtime_t *runtime, chart_view_runtime_info_t *out_info);

CHART_RUNTIME_EXPORT chart_view_status_t
  chart_view_runtime_get_loaded_chart_info(const chart_view_runtime_t *runtime,
                                           chart_view_loaded_chart_info_t *out_info);

CHART_RUNTIME_EXPORT chart_view_status_t
  chart_view_runtime_get_viewport(const chart_view_runtime_t *runtime,
                                  chart_view_viewport_t *out_viewport);

/* -- Viewport ---------------------------------------------------- */

CHART_RUNTIME_EXPORT chart_view_status_t
  chart_view_runtime_set_viewport(chart_view_runtime_t *runtime, const chart_view_viewport_t *viewport);

CHART_RUNTIME_EXPORT chart_view_status_t
  chart_view_runtime_step_zoom(chart_view_runtime_t *runtime,
                               int32_t step_count,
                               chart_view_zoom_result_t *out_result);

/* -- SENC data --------------------------------------------------- */

CHART_RUNTIME_EXPORT chart_view_status_t
  chart_view_runtime_load_senc(chart_view_runtime_t *runtime,
                               const void *data,
                               uint32_t size);

/* -- Open source chart ------------------------------------------- */

CHART_RUNTIME_EXPORT chart_view_status_t
  chart_view_runtime_open_chart_file(chart_view_runtime_t *runtime,
                                     const char *path,
                                     chart_view_chart_source_type_t source_type);

CHART_RUNTIME_EXPORT chart_view_status_t
  chart_view_runtime_open_chart_directory(chart_view_runtime_t *runtime,
                                          const char *path);

/* -- Render ------------------------------------------------------ */

CHART_RUNTIME_EXPORT chart_view_status_t
  chart_view_runtime_render_frame(chart_view_runtime_t *runtime,
                                  chart_view_render_frame_result_t *out_result);

CHART_RUNTIME_EXPORT chart_view_status_t
  chart_view_runtime_get_frame_buffer_info(const chart_view_runtime_t *runtime,
                                           chart_view_frame_buffer_info_t *out_info);

CHART_RUNTIME_EXPORT chart_view_status_t
  chart_view_runtime_copy_frame_rgba(const chart_view_runtime_t *runtime,
                                     void *dst_rgba,
                                     uint32_t dst_size_bytes);

#ifdef __cplusplus
}
#endif

#endif
