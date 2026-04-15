#ifndef CHART_VIEW_RUNTIME_CHART_RUNTIME_TYPES_H
#define CHART_VIEW_RUNTIME_CHART_RUNTIME_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -- ABI version ------------------------------------------------- */

enum
{
  CHART_VIEW_RUNTIME_ABI_VERSION = 1u
};

/* -- Opaque handle ----------------------------------------------- */

typedef struct chart_view_runtime chart_view_runtime_t;

/* -- Status codes ------------------------------------------------ */

typedef enum chart_view_status
{
  chart_view_status_ok = 0,
  chart_view_status_invalid_argument = 1,
  chart_view_status_allocation_failure = 2,
  chart_view_status_not_initialized = 3,
  chart_view_status_already_initialized = 4,
  chart_view_status_not_implemented = 5,
  chart_view_status_io_error = 6,
  chart_view_status_invalid_format = 7
} chart_view_status_t;

/* -- Feature flags ----------------------------------------------- */

typedef enum chart_view_feature_flag
{
  CHART_VIEW_FEATURE_QT_RHI = 1u << 0u,
  CHART_VIEW_FEATURE_S57 = 1u << 1u,
  CHART_VIEW_FEATURE_CM93 = 1u << 2u,
  CHART_VIEW_FEATURE_S101 = 1u << 3u
} chart_view_feature_flag_t;

/* -- Chart source type ------------------------------------------- */

typedef enum chart_view_chart_source_type
{
  chart_view_chart_source_unknown = 0,
  chart_view_chart_source_s57 = 1,
  chart_view_chart_source_cm93 = 2,
  chart_view_chart_source_s101 = 3
} chart_view_chart_source_type_t;

/* -- Runtime info DTO -------------------------------------------- */

typedef struct chart_view_runtime_info
{
  uint32_t abi_version;
  const char *project_name;
  const char *project_version;
  const char *git_sha;
  uint32_t enabled_feature_flags;
} chart_view_runtime_info_t;

/* -- Viewport DTO ------------------------------------------------ */

typedef struct chart_view_viewport
{
  double center_lon;
  double center_lat;
  double scale_denominator;
  double rotation_rad;
  int32_t pixel_width;
  int32_t pixel_height;
} chart_view_viewport_t;

/* -- Loaded chart summary DTO ------------------------------------- */

typedef struct chart_view_loaded_chart_info
{
  chart_view_chart_source_type_t source_type;
  uint32_t feature_count;
  double min_lon;
  double min_lat;
  double max_lon;
  double max_lat;
} chart_view_loaded_chart_info_t;

/* -- Render frame result DTO ------------------------------------- */

typedef struct chart_view_render_frame_result
{
  uint32_t points_rendered;
  uint32_t lines_rendered;
  uint32_t areas_rendered;
  uint32_t total_vertices;
} chart_view_render_frame_result_t;

/* -- Frame buffer DTO ------------------------------------------- */

typedef struct chart_view_frame_buffer_info
{
  uint32_t pixel_width;
  uint32_t pixel_height;
  uint32_t stride_bytes;
  uint32_t rgba_size_bytes;
} chart_view_frame_buffer_info_t;

#ifdef __cplusplus
}
#endif

#endif
