#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chart_view/runtime/chart_runtime.h>

#include "senc/senc_writer.hpp"
#include "chart_data/feature_chart_dataset.hpp"
#include "chart_data/geometry.hpp"

#include <filesystem>
#include <fstream>
#include <array>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#include <cstdlib>

namespace {
struct DebugCrtReportRedirect
{
  DebugCrtReportRedirect()
  {
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
  }
} g_debugCrtReportRedirect;
}// namespace
#endif

TEST_CASE("runtime handle can be created and queried", "[runtime]")
{
  chart_view_runtime_t *runtime = nullptr;

  REQUIRE(chart_view_runtime_create(&runtime) == chart_view_status_ok);
  REQUIRE(runtime != nullptr);

  chart_view_runtime_info_t info{};
  REQUIRE(chart_view_runtime_get_info(runtime, &info) == chart_view_status_ok);
  REQUIRE(info.abi_version == CHART_VIEW_RUNTIME_ABI_VERSION);
  REQUIRE(std::string_view(info.project_name) == "chart_view");
  REQUIRE(std::string_view(info.project_version) == std::string_view(chart_view_runtime_version_string()));
  REQUIRE((info.enabled_feature_flags & CHART_VIEW_FEATURE_S57) != 0U);

  chart_view_runtime_destroy(runtime);
}

TEST_CASE("runtime rejects invalid arguments", "[runtime]")
{
  REQUIRE(chart_view_runtime_create(nullptr) == chart_view_status_invalid_argument);
  REQUIRE(chart_view_runtime_get_info(nullptr, nullptr) == chart_view_status_invalid_argument);
  REQUIRE(chart_view_runtime_get_loaded_chart_info(nullptr, nullptr) == chart_view_status_invalid_argument);
}

TEST_CASE("runtime lifecycle: create -> initialize -> shutdown -> destroy", "[runtime]")
{
  chart_view_runtime_t *runtime = nullptr;
  REQUIRE(chart_view_runtime_create(&runtime) == chart_view_status_ok);

  REQUIRE(chart_view_runtime_initialize(runtime) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_initialize(runtime) == chart_view_status_already_initialized);

  REQUIRE(chart_view_runtime_shutdown(runtime) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_shutdown(runtime) == chart_view_status_not_initialized);

  chart_view_runtime_destroy(runtime);
}

TEST_CASE("initialize and shutdown reject null handle", "[runtime]")
{
  REQUIRE(chart_view_runtime_initialize(nullptr) == chart_view_status_invalid_argument);
  REQUIRE(chart_view_runtime_shutdown(nullptr) == chart_view_status_invalid_argument);
}

TEST_CASE("types header provides viewport struct", "[runtime][types]")
{
  chart_view_viewport_t vp{};
  vp.center_lon = 121.5;
  vp.center_lat = 31.2;
  vp.scale_denominator = 50000.0;
  vp.rotation_rad = 0.0;
  vp.pixel_width = 1280;
  vp.pixel_height = 720;

  REQUIRE(vp.pixel_width == 1280);
  REQUIRE(vp.pixel_height == 720);
}

TEST_CASE("types header provides chart source type enum", "[runtime][types]")
{
  REQUIRE(chart_view_chart_source_unknown == 0);
  REQUIRE(chart_view_chart_source_s57 == 1);
  REQUIRE(chart_view_chart_source_cm93 == 2);
  REQUIRE(chart_view_chart_source_s101 == 3);
}

TEST_CASE("types header provides S-52 mariner-settings DTOs", "[runtime][types]")
{
  chart_view_s52_mariner_settings_t settings{};
  settings.palette = chart_view_s52_palette_day;
  settings.display_category = chart_view_s52_display_standard;
  settings.show_text = 1U;
  settings.show_soundings = 1U;
  settings.simplified_points = 0U;
  settings.two_shades = 0U;
  settings.safety_contour_m = 30.0;
  settings.safety_depth_m = 30.0;
  settings.shallow_contour_m = 2.0;
  settings.deep_contour_m = 30.0;
  settings.shallow_pattern = 1U;
  settings.full_sector_lights = 0U;
  settings.symbolized_boundaries = 1U;
  settings.honor_scamin = 1U;

  chart_view_s57_class_filter_t classFilter{"WRECKS", 1U};
  chart_view_s52_rule_filter_t ruleFilter{"s52_point_wrecks_point_danger01_point_danger", 0U};
  chart_view_s52_rule_descriptor_t ruleDescriptor{};

  REQUIRE(settings.palette == chart_view_s52_palette_day);
  REQUIRE(settings.display_category == chart_view_s52_display_standard);
  REQUIRE(std::string_view(classFilter.object_acronym) == "WRECKS");
  REQUIRE(std::string_view(ruleFilter.rule_id) == "s52_point_wrecks_point_danger01_point_danger");
  REQUIRE(ruleDescriptor.view_group == 0U);
}

TEST_CASE("types header provides feature query and inspection DTOs", "[runtime][types]")
{
  chart_view_feature_query_t query{};
  query.lon = 120.1;
  query.lat = 31.2;
  query.tolerance_m = 25.0;
  query.max_results = 8U;

  chart_view_feature_summary_t summary{};
  summary.runtime_feature_token = 0U;
  summary.feature_id = 0U;
  summary.source_type = chart_view_chart_source_s57;
  summary.geometry_type = chart_view_feature_geometry_point;
  summary.display_category = chart_view_s52_display_standard;

  REQUIRE(query.tolerance_m == Catch::Approx(25.0));
  REQUIRE(query.max_results == 8U);
  REQUIRE(summary.geometry_type == chart_view_feature_geometry_point);
  REQUIRE(summary.display_category == chart_view_s52_display_standard);
}

// -- Render frame API tests --

static std::vector<std::uint8_t> buildTestSenc()
{
  using namespace chart_view::runtime::chart_data;
  using namespace chart_view::runtime::senc;

  FeatureChartDataset ds;
  DatasetMeta meta;
  meta.name = "api_test";
  meta.sourceType = chart_view_chart_source_s57;
  meta.extent = {-1.0, 50.0, 1.0, 52.0};
  ds.setMeta(std::move(meta));

  Feature f0;
  f0.id = 1;
  f0.classCode = 100;
  f0.geometry = PointGeometry{{0.0, 51.0}};
  ds.addFeature(std::move(f0));

  Feature f1;
  f1.id = 2;
  f1.classCode = 200;
  f1.geometry = LineGeometry{{{-0.5, 50.5}, {0.5, 51.5}}};
  ds.addFeature(std::move(f1));

  SencWriter writer;
  return writer.write(ds);
}

static std::vector<std::uint8_t> buildQueryableSenc()
{
  using namespace chart_view::runtime::chart_data;
  using namespace chart_view::runtime::senc;

  FeatureChartDataset ds;
  DatasetMeta meta;
  meta.name = "query_test";
  meta.sourceType = chart_view_chart_source_s57;
  meta.extent = {-1.0, 50.0, 1.0, 52.0};
  ds.setMeta(std::move(meta));

  Feature wreck;
  wreck.id = 101;
  wreck.classCode = 159;
  wreck.classAcronym = "WRECKS";
  wreck.geometry = PointGeometry{{0.0, 51.0}};
  wreck.attributes["OBJNAM"] = std::string("Old wreck");
  wreck.attributes["NOBJNM"] = std::string("\xE6\xB2\x89\xE8\x88\xB9\x41");
  ds.addFeature(std::move(wreck));

  Feature depthArea;
  depthArea.id = 202;
  depthArea.classCode = 42;
  depthArea.classAcronym = "DEPARE";
  depthArea.geometry = AreaGeometry{{{-0.05, 50.95}, {0.05, 50.95}, {0.05, 51.05}, {-0.05, 51.05}}, {}};
  depthArea.attributes["DRVAL1"] = 1.0;
  depthArea.attributes["DRVAL2"] = 4.0;
  depthArea.attributes["OBJNAM"] = std::string("Shallow area");
  ds.addFeature(std::move(depthArea));

  SencWriter writer;
  return writer.write(ds);
}

static void writeTextFile(const std::filesystem::path &path, std::string_view text)
{
  std::ofstream out(path, std::ios::binary);
  REQUIRE(out.good());
  out.write(text.data(), static_cast<std::streamsize>(text.size()));
  REQUIRE(out.good());
}

TEST_CASE("set_viewport rejects null arguments", "[runtime][api]")
{
  chart_view_runtime_t *rt = nullptr;
  REQUIRE(chart_view_runtime_create(&rt) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_initialize(rt) == chart_view_status_ok);

  REQUIRE(chart_view_runtime_set_viewport(nullptr, nullptr) == chart_view_status_invalid_argument);
  REQUIRE(chart_view_runtime_set_viewport(rt, nullptr) == chart_view_status_invalid_argument);

  chart_view_runtime_destroy(rt);
}

TEST_CASE("set_viewport requires initialized runtime", "[runtime][api]")
{
  chart_view_runtime_t *rt = nullptr;
  REQUIRE(chart_view_runtime_create(&rt) == chart_view_status_ok);

  chart_view_viewport_t vp{};
  vp.center_lon = 0.0;
  vp.center_lat = 51.0;
  vp.scale_denominator = 50000.0;
  vp.pixel_width = 800;
  vp.pixel_height = 600;

  REQUIRE(chart_view_runtime_set_viewport(rt, &vp) == chart_view_status_not_initialized);

  chart_view_runtime_destroy(rt);
}

TEST_CASE("get_viewport returns the current runtime viewport", "[runtime][api]")
{
  chart_view_runtime_t *rt = nullptr;
  REQUIRE(chart_view_runtime_create(&rt) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_initialize(rt) == chart_view_status_ok);

  chart_view_viewport_t setVp{};
  setVp.center_lon = 12.5;
  setVp.center_lat = 48.1;
  setVp.scale_denominator = 75000.0;
  setVp.rotation_rad = 0.25;
  setVp.pixel_width = 640;
  setVp.pixel_height = 480;
  REQUIRE(chart_view_runtime_set_viewport(rt, &setVp) == chart_view_status_ok);

  chart_view_viewport_t getVp{};
  REQUIRE(chart_view_runtime_get_viewport(rt, &getVp) == chart_view_status_ok);
  REQUIRE(getVp.center_lon == Catch::Approx(setVp.center_lon));
  REQUIRE(getVp.center_lat == Catch::Approx(setVp.center_lat));
  REQUIRE(getVp.scale_denominator == Catch::Approx(setVp.scale_denominator));
  REQUIRE(getVp.rotation_rad == Catch::Approx(setVp.rotation_rad));
  REQUIRE(getVp.pixel_width == setVp.pixel_width);
  REQUIRE(getVp.pixel_height == setVp.pixel_height);

  chart_view_runtime_destroy(rt);
}

TEST_CASE("step_zoom updates viewport scale through the runtime C API", "[runtime][api]")
{
  chart_view_runtime_t *rt = nullptr;
  REQUIRE(chart_view_runtime_create(&rt) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_initialize(rt) == chart_view_status_ok);

  chart_view_viewport_t vp{};
  vp.center_lon = 0.0;
  vp.center_lat = 51.0;
  vp.scale_denominator = 100000.0;
  vp.pixel_width = 800;
  vp.pixel_height = 600;
  REQUIRE(chart_view_runtime_set_viewport(rt, &vp) == chart_view_status_ok);

  chart_view_zoom_result_t zoom{};
  REQUIRE(chart_view_runtime_step_zoom(rt, 1, &zoom) == chart_view_status_ok);
  REQUIRE(zoom.viewport.scale_denominator == Catch::Approx(80000.0));
  REQUIRE(zoom.resolved_scale_denominator == Catch::Approx(80000.0));

  chart_view_viewport_t updatedVp{};
  REQUIRE(chart_view_runtime_get_viewport(rt, &updatedVp) == chart_view_status_ok);
  REQUIRE(updatedVp.scale_denominator == Catch::Approx(80000.0));

  chart_view_runtime_destroy(rt);
}

TEST_CASE("load_senc rejects null/empty data", "[runtime][api]")
{
  chart_view_runtime_t *rt = nullptr;
  REQUIRE(chart_view_runtime_create(&rt) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_initialize(rt) == chart_view_status_ok);

  REQUIRE(chart_view_runtime_load_senc(nullptr, nullptr, 0) == chart_view_status_invalid_argument);
  REQUIRE(chart_view_runtime_load_senc(rt, nullptr, 0) == chart_view_status_invalid_argument);

  std::uint8_t dummy = 0;
  REQUIRE(chart_view_runtime_load_senc(rt, &dummy, 0) == chart_view_status_invalid_argument);

  chart_view_runtime_destroy(rt);
}

TEST_CASE("render_frame rejects null arguments", "[runtime][api]")
{
  REQUIRE(chart_view_runtime_render_frame(nullptr, nullptr) == chart_view_status_invalid_argument);
}

TEST_CASE("open_chart_file validates arguments and lifecycle", "[runtime][api]")
{
  chart_view_runtime_t *rt = nullptr;
  REQUIRE(chart_view_runtime_create(&rt) == chart_view_status_ok);

  REQUIRE(chart_view_runtime_open_chart_file(nullptr, nullptr, chart_view_chart_source_unknown)
          == chart_view_status_invalid_argument);
  REQUIRE(chart_view_runtime_open_chart_file(rt, nullptr, chart_view_chart_source_s57)
          == chart_view_status_invalid_argument);
  REQUIRE(chart_view_runtime_open_chart_file(rt, "", chart_view_chart_source_s57)
          == chart_view_status_invalid_argument);
  REQUIRE(chart_view_runtime_open_chart_file(rt, "dummy.000", chart_view_chart_source_s57)
          == chart_view_status_not_initialized);

  chart_view_runtime_destroy(rt);
}

TEST_CASE("open_chart_file routes by extension and loads S-101 scaffold", "[runtime][api]")
{
  const auto tempPath = std::filesystem::temp_directory_path() / "chart_view_open_chart_smoke.101";

  {
    std::ofstream out(tempPath, std::ios::binary);
    REQUIRE(out.good());
    const std::vector<std::uint8_t> payload(64, 0x55);
    out.write(reinterpret_cast<const char *>(payload.data()), static_cast<std::streamsize>(payload.size()));
    REQUIRE(out.good());
  }

  chart_view_runtime_t *rt = nullptr;
  REQUIRE(chart_view_runtime_create(&rt) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_initialize(rt) == chart_view_status_ok);

  REQUIRE(chart_view_runtime_open_chart_file(
            rt,
            tempPath.string().c_str(),
            chart_view_chart_source_unknown)
          == chart_view_status_ok);

  chart_view_loaded_chart_info_t info{};
  REQUIRE(chart_view_runtime_get_loaded_chart_info(rt, &info) == chart_view_status_ok);
  REQUIRE(info.source_type == chart_view_chart_source_s101);

  chart_view_render_frame_result_t result{};
  REQUIRE(chart_view_runtime_render_frame(rt, &result) == chart_view_status_ok);

  chart_view_runtime_shutdown(rt);
  chart_view_runtime_destroy(rt);

  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_CASE("open_chart_directory builds a multi-chart runtime view from source charts", "[runtime][api]")
{
  static constexpr std::string_view kChartA = R"(S101SMOKE
name=runtime_dir_chart_a
native_scale=12000
point=121.8006,31.2301
line=121.8002,31.2298;121.8013,31.2304;121.8020,31.2299
area=121.8004,31.2300;121.8014,31.2300;121.8014,31.2308;121.8004,31.2308
)";

  static constexpr std::string_view kChartB = R"(S101SMOKE
name=runtime_dir_chart_b
native_scale=15000
point=121.8016,31.2302
line=121.8010,31.2299;121.8022,31.2305;121.8031,31.2301
area=121.8012,31.2301;121.8025,31.2301;121.8025,31.2310;121.8012,31.2310
)";

  const auto tempRoot = std::filesystem::temp_directory_path() / "chart_view_open_chart_directory_smoke";
  std::error_code ec;
  std::filesystem::remove_all(tempRoot, ec);
  REQUIRE(std::filesystem::create_directories(tempRoot));

  writeTextFile(tempRoot / "chart_a.101", kChartA);
  writeTextFile(tempRoot / "chart_b.101", kChartB);

  chart_view_runtime_t *rt = nullptr;
  REQUIRE(chart_view_runtime_create(&rt) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_initialize(rt) == chart_view_status_ok);

  REQUIRE(chart_view_runtime_open_chart_directory(rt, tempRoot.string().c_str()) == chart_view_status_ok);

  chart_view_loaded_chart_info_t info{};
  REQUIRE(chart_view_runtime_get_loaded_chart_info(rt, &info) == chart_view_status_ok);
  REQUIRE(info.source_type == chart_view_chart_source_s101);
  REQUIRE(info.feature_count > 0U);
  REQUIRE(info.min_lon < info.max_lon);
  REQUIRE(info.min_lat < info.max_lat);

  chart_view_render_frame_result_t result{};
  REQUIRE(chart_view_runtime_render_frame(rt, &result) == chart_view_status_ok);
  REQUIRE(result.total_vertices > 0U);

  chart_view_runtime_shutdown(rt);
  chart_view_runtime_destroy(rt);

  std::filesystem::remove_all(tempRoot, ec);
}

TEST_CASE("runtime mariner settings roundtrip through narrow C API", "[runtime][api][mariner]")
{
  chart_view_runtime_t *rt = nullptr;
  REQUIRE(chart_view_runtime_create(&rt) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_initialize(rt) == chart_view_status_ok);

  chart_view_s52_mariner_settings_t defaults{};
  REQUIRE(chart_view_runtime_get_s52_mariner_settings(rt, &defaults) == chart_view_status_ok);
  REQUIRE(defaults.palette == chart_view_s52_palette_day);
  REQUIRE(defaults.display_category == chart_view_s52_display_standard);
  REQUIRE(defaults.show_text == 1U);
  REQUIRE(defaults.show_soundings == 1U);

  chart_view_s52_mariner_settings_t desired{};
  desired.palette = chart_view_s52_palette_night;
  desired.display_category = chart_view_s52_display_all;
  desired.show_text = 0U;
  desired.show_soundings = 0U;
  desired.simplified_points = 1U;
  desired.two_shades = 1U;
  desired.safety_contour_m = 12.0;
  desired.safety_depth_m = 11.5;
  desired.shallow_contour_m = 3.0;
  desired.deep_contour_m = 40.0;
  desired.shallow_pattern = 0U;
  desired.full_sector_lights = 1U;
  desired.symbolized_boundaries = 0U;
  desired.honor_scamin = 0U;
  REQUIRE(chart_view_runtime_set_s52_mariner_settings(rt, &desired) == chart_view_status_ok);

  chart_view_s52_mariner_settings_t readback{};
  REQUIRE(chart_view_runtime_get_s52_mariner_settings(rt, &readback) == chart_view_status_ok);
  REQUIRE(readback.palette == chart_view_s52_palette_night);
  REQUIRE(readback.display_category == chart_view_s52_display_all);
  REQUIRE(readback.show_text == 0U);
  REQUIRE(readback.show_soundings == 0U);
  REQUIRE(readback.simplified_points == 1U);
  REQUIRE(readback.two_shades == 1U);
  REQUIRE(readback.safety_contour_m == Catch::Approx(12.0));
  REQUIRE(readback.safety_depth_m == Catch::Approx(11.5));
  REQUIRE(readback.shallow_contour_m == Catch::Approx(3.0));
  REQUIRE(readback.deep_contour_m == Catch::Approx(40.0));
  REQUIRE(readback.shallow_pattern == 0U);
  REQUIRE(readback.full_sector_lights == 1U);
  REQUIRE(readback.symbolized_boundaries == 0U);
  REQUIRE(readback.honor_scamin == 0U);

  desired.palette = static_cast<chart_view_s52_color_palette_t>(99);
  REQUIRE(chart_view_runtime_set_s52_mariner_settings(rt, &desired) == chart_view_status_invalid_argument);

  chart_view_runtime_destroy(rt);
}

TEST_CASE("runtime mariner settings affect palette-sensitive frame output", "[runtime][api][mariner][render]")
{
  chart_view_runtime_t *rt = nullptr;
  REQUIRE(chart_view_runtime_create(&rt) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_initialize(rt) == chart_view_status_ok);

  chart_view_viewport_t vp{};
  vp.center_lon = 0.0;
  vp.center_lat = 51.0;
  vp.scale_denominator = 100000.0;
  vp.pixel_width = 800;
  vp.pixel_height = 600;
  REQUIRE(chart_view_runtime_set_viewport(rt, &vp) == chart_view_status_ok);

  auto senc = buildTestSenc();
  REQUIRE(chart_view_runtime_load_senc(rt, senc.data(), static_cast<std::uint32_t>(senc.size()))
          == chart_view_status_ok);

  chart_view_render_frame_result_t renderResult{};
  REQUIRE(chart_view_runtime_render_frame(rt, &renderResult) == chart_view_status_ok);

  chart_view_frame_buffer_info_t frameInfo{};
  REQUIRE(chart_view_runtime_get_frame_buffer_info(rt, &frameInfo) == chart_view_status_ok);
  REQUIRE(frameInfo.rgba_size_bytes > 0U);

  std::vector<std::uint8_t> dayRgba(frameInfo.rgba_size_bytes, 0U);
  REQUIRE(chart_view_runtime_copy_frame_rgba(rt, dayRgba.data(), static_cast<std::uint32_t>(dayRgba.size()))
          == chart_view_status_ok);

  chart_view_s52_mariner_settings_t settings{};
  REQUIRE(chart_view_runtime_get_s52_mariner_settings(rt, &settings) == chart_view_status_ok);
  settings.palette = chart_view_s52_palette_night;
  REQUIRE(chart_view_runtime_set_s52_mariner_settings(rt, &settings) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_render_frame(rt, &renderResult) == chart_view_status_ok);

  std::vector<std::uint8_t> nightRgba(frameInfo.rgba_size_bytes, 0U);
  REQUIRE(chart_view_runtime_copy_frame_rgba(rt, nightRgba.data(), static_cast<std::uint32_t>(nightRgba.size()))
          == chart_view_status_ok);

  const std::array<std::uint8_t, 4> dayPixel{
    dayRgba[0],
    dayRgba[1],
    dayRgba[2],
    dayRgba[3]};
  const std::array<std::uint8_t, 4> nightPixel{
    nightRgba[0],
    nightRgba[1],
    nightRgba[2],
    nightRgba[3]};
  REQUIRE(dayPixel != nightPixel);

  chart_view_runtime_destroy(rt);
}

TEST_CASE("runtime class and rule filters roundtrip through narrow C API", "[runtime][api][filters]")
{
  chart_view_runtime_t *rt = nullptr;
  REQUIRE(chart_view_runtime_create(&rt) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_initialize(rt) == chart_view_status_ok);

  const std::array classFilters{
    chart_view_s57_class_filter_t{"wrecks", 1U},
    chart_view_s57_class_filter_t{"boylat", 0U}};
  REQUIRE(chart_view_runtime_set_s57_class_filters(
            rt,
            classFilters.data(),
            static_cast<std::uint32_t>(classFilters.size()))
          == chart_view_status_ok);

  std::uint32_t classCount = 0;
  REQUIRE(chart_view_runtime_get_s57_class_filters(rt, nullptr, &classCount) == chart_view_status_ok);
  REQUIRE(classCount == classFilters.size());

  std::array<chart_view_s57_class_filter_t, 2> classReadback{};
  REQUIRE(chart_view_runtime_get_s57_class_filters(rt, classReadback.data(), &classCount)
          == chart_view_status_ok);
  REQUIRE(std::string_view(classReadback[0].object_acronym) == "WRECKS");
  REQUIRE(classReadback[0].enabled == 1U);
  REQUIRE(std::string_view(classReadback[1].object_acronym) == "BOYLAT");
  REQUIRE(classReadback[1].enabled == 0U);

  const std::array ruleFilters{
    chart_view_s52_rule_filter_t{"S52_POINT_WRECKS_POINT_DANGER01_POINT_DANGER", 0U},
    chart_view_s52_rule_filter_t{"s52_line_coalne_line_coastline01_line_coastline", 1U}};
  REQUIRE(chart_view_runtime_set_s52_rule_filters(
            rt,
            ruleFilters.data(),
            static_cast<std::uint32_t>(ruleFilters.size()))
          == chart_view_status_ok);

  std::uint32_t ruleCount = 0;
  REQUIRE(chart_view_runtime_get_s52_rule_filters(rt, nullptr, &ruleCount) == chart_view_status_ok);
  REQUIRE(ruleCount == ruleFilters.size());

  std::array<chart_view_s52_rule_filter_t, 2> ruleReadback{};
  REQUIRE(chart_view_runtime_get_s52_rule_filters(rt, ruleReadback.data(), &ruleCount)
          == chart_view_status_ok);
  REQUIRE(std::string_view(ruleReadback[0].rule_id) == "s52_point_wrecks_point_danger01_point_danger");
  REQUIRE(ruleReadback[0].enabled == 0U);
  REQUIRE(std::string_view(ruleReadback[1].rule_id) == "s52_line_coalne_line_coastline01_line_coastline");
  REQUIRE(ruleReadback[1].enabled == 1U);

  chart_view_runtime_destroy(rt);
}

TEST_CASE("runtime applies class and rule selection controls to query and render behavior", "[runtime][api][filters][selection]")
{
  chart_view_runtime_t *rt = nullptr;
  REQUIRE(chart_view_runtime_create(&rt) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_initialize(rt) == chart_view_status_ok);

  auto senc = buildQueryableSenc();
  REQUIRE(chart_view_runtime_load_senc(rt, senc.data(), static_cast<std::uint32_t>(senc.size()))
          == chart_view_status_ok);

  chart_view_viewport_t viewport{};
  viewport.center_lon = 0.0;
  viewport.center_lat = 51.0;
  viewport.scale_denominator = 40000.0;
  viewport.pixel_width = 800;
  viewport.pixel_height = 600;
  REQUIRE(chart_view_runtime_set_viewport(rt, &viewport) == chart_view_status_ok);

  chart_view_feature_query_t query{};
  query.lon = 0.0;
  query.lat = 51.0;
  query.tolerance_m = 40.0;
  query.max_results = 8U;

  std::array<chart_view_feature_summary_t, 8> baselineSummaries{};
  std::uint32_t baselineCount = static_cast<std::uint32_t>(baselineSummaries.size());
  REQUIRE(chart_view_runtime_query_features_at_point(
            rt,
            &query,
            baselineSummaries.data(),
            &baselineCount)
          == chart_view_status_ok);
  REQUIRE(baselineCount >= 2U);

  const auto baselineWreck = std::ranges::find_if(
    baselineSummaries.begin(),
    baselineSummaries.begin() + static_cast<std::ptrdiff_t>(baselineCount),
    [](const chart_view_feature_summary_t &summary) {
      return summary.object_acronym != nullptr && std::string_view(summary.object_acronym) == "WRECKS";
    });
  REQUIRE(baselineWreck != baselineSummaries.begin() + static_cast<std::ptrdiff_t>(baselineCount));
  REQUIRE(baselineWreck->active_rule_id != nullptr);
  REQUIRE_FALSE(std::string_view(baselineWreck->active_rule_id).empty());
  REQUIRE(baselineWreck->suppressed == 0U);

  const auto baselineDepthArea = std::ranges::find_if(
    baselineSummaries.begin(),
    baselineSummaries.begin() + static_cast<std::ptrdiff_t>(baselineCount),
    [](const chart_view_feature_summary_t &summary) {
      return summary.object_acronym != nullptr && std::string_view(summary.object_acronym) == "DEPARE";
    });
  REQUIRE(
    baselineDepthArea != baselineSummaries.begin() + static_cast<std::ptrdiff_t>(baselineCount));
  REQUIRE(baselineDepthArea->active_rule_id != nullptr);
  REQUIRE_FALSE(std::string_view(baselineDepthArea->active_rule_id).empty());
  REQUIRE(baselineDepthArea->suppressed == 0U);

  const auto wreckRuleId = std::string(baselineWreck->active_rule_id);
  const auto depthAreaRuleId = std::string(baselineDepthArea->active_rule_id);

  const std::array classFilters{
    chart_view_s57_class_filter_t{"DEPARE", 0U}};
  REQUIRE(chart_view_runtime_set_s57_class_filters(
            rt,
            classFilters.data(),
            static_cast<std::uint32_t>(classFilters.size()))
          == chart_view_status_ok);

  const std::array ruleFilters{
    chart_view_s52_rule_filter_t{wreckRuleId.c_str(), 0U},
    chart_view_s52_rule_filter_t{depthAreaRuleId.c_str(), 1U}};
  REQUIRE(chart_view_runtime_set_s52_rule_filters(
            rt,
            ruleFilters.data(),
            static_cast<std::uint32_t>(ruleFilters.size()))
          == chart_view_status_ok);

  std::array<chart_view_feature_summary_t, 8> summaries{};
  std::uint32_t resultCount = static_cast<std::uint32_t>(summaries.size());
  REQUIRE(chart_view_runtime_query_features_at_point(rt, &query, summaries.data(), &resultCount)
          == chart_view_status_ok);
  REQUIRE(resultCount >= 2U);

  const auto wreck = std::ranges::find_if(
    summaries.begin(),
    summaries.begin() + static_cast<std::ptrdiff_t>(resultCount),
    [](const chart_view_feature_summary_t &summary) {
      return summary.object_acronym != nullptr && std::string_view(summary.object_acronym) == "WRECKS";
    });
  REQUIRE(wreck != summaries.begin() + static_cast<std::ptrdiff_t>(resultCount));
  REQUIRE(wreck->suppressed == 1U);
  REQUIRE(wreck->active_rule_id != nullptr);
  REQUIRE(std::string_view(wreck->active_rule_id) == wreckRuleId);

  const auto depthArea = std::ranges::find_if(
    summaries.begin(),
    summaries.begin() + static_cast<std::ptrdiff_t>(resultCount),
    [](const chart_view_feature_summary_t &summary) {
      return summary.object_acronym != nullptr && std::string_view(summary.object_acronym) == "DEPARE";
    });
  REQUIRE(depthArea != summaries.begin() + static_cast<std::ptrdiff_t>(resultCount));
  REQUIRE(depthArea->suppressed == 1U);
  REQUIRE(depthArea->active_rule_id != nullptr);
  REQUIRE(std::string_view(depthArea->active_rule_id) == depthAreaRuleId);

  chart_view_render_frame_result_t renderResult{};
  REQUIRE(chart_view_runtime_render_frame(rt, &renderResult) == chart_view_status_ok);
  REQUIRE(renderResult.points_rendered == 0U);
  REQUIRE(renderResult.areas_rendered == 0U);
  REQUIRE(renderResult.total_vertices == 0U);

  chart_view_runtime_destroy(rt);
}

TEST_CASE("runtime enumerates compiled S-52 rule descriptors through the C API", "[runtime][api][rules]")
{
  chart_view_runtime_t *rt = nullptr;
  REQUIRE(chart_view_runtime_create(&rt) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_initialize(rt) == chart_view_status_ok);

  std::uint32_t ruleCount = 0;
  REQUIRE(chart_view_runtime_enumerate_s52_rules(rt, nullptr, &ruleCount) == chart_view_status_ok);
  REQUIRE(ruleCount > 0U);

  std::vector<chart_view_s52_rule_descriptor_t> descriptors(ruleCount);
  REQUIRE(chart_view_runtime_enumerate_s52_rules(rt, descriptors.data(), &ruleCount)
          == chart_view_status_ok);
  REQUIRE(ruleCount == descriptors.size());

  const auto wreckRule = std::ranges::find_if(
    descriptors,
    [](const chart_view_s52_rule_descriptor_t &descriptor) {
      return std::string_view(descriptor.object_acronym) == "WRECKS";
    });
  REQUIRE(wreckRule != descriptors.end());
  REQUIRE(wreckRule->rule_id != nullptr);
  REQUIRE(wreckRule->label != nullptr);
  REQUIRE(std::string_view(wreckRule->label).find("WRECKS") != std::string_view::npos);
  REQUIRE(wreckRule->display_category == chart_view_s52_display_standard);
  REQUIRE(wreckRule->view_group > 0U);

  chart_view_runtime_destroy(rt);
}

TEST_CASE("runtime queries feature summaries at a point through the C API", "[runtime][api][query]")
{
  chart_view_runtime_t *rt = nullptr;
  REQUIRE(chart_view_runtime_create(&rt) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_initialize(rt) == chart_view_status_ok);

  auto senc = buildQueryableSenc();
  REQUIRE(chart_view_runtime_load_senc(rt, senc.data(), static_cast<std::uint32_t>(senc.size()))
          == chart_view_status_ok);

  chart_view_viewport_t viewport{};
  viewport.center_lon = 0.0;
  viewport.center_lat = 51.0;
  viewport.scale_denominator = 40000.0;
  viewport.pixel_width = 800;
  viewport.pixel_height = 600;
  REQUIRE(chart_view_runtime_set_viewport(rt, &viewport) == chart_view_status_ok);

  chart_view_feature_query_t query{};
  query.lon = 0.0;
  query.lat = 51.0;
  query.tolerance_m = 40.0;
  query.max_results = 8U;

  std::uint32_t resultCount = 0;
  REQUIRE(chart_view_runtime_query_features_at_point(rt, &query, nullptr, &resultCount)
          == chart_view_status_ok);
  REQUIRE(resultCount >= 2U);

  std::vector<chart_view_feature_summary_t> summaries(resultCount);
  REQUIRE(chart_view_runtime_query_features_at_point(rt, &query, summaries.data(), &resultCount)
          == chart_view_status_ok);
  REQUIRE(resultCount == summaries.size());

  const auto wreck = std::ranges::find_if(
    summaries,
    [](const chart_view_feature_summary_t &summary) {
      return summary.object_acronym != nullptr && std::string_view(summary.object_acronym) == "WRECKS";
    });
  REQUIRE(wreck != summaries.end());
  REQUIRE(wreck->source_type == chart_view_chart_source_s57);
  REQUIRE(wreck->geometry_type == chart_view_feature_geometry_point);
  REQUIRE(wreck->feature_id == 101U);
  REQUIRE(wreck->dataset_name != nullptr);
  REQUIRE(std::string_view(wreck->dataset_name) == "query_test");
  REQUIRE(wreck->primary_name != nullptr);
  REQUIRE(std::string_view(wreck->primary_name) == std::string_view("\xE6\xB2\x89\xE8\x88\xB9\x41"));
  REQUIRE(wreck->name_source_attribute != nullptr);
  REQUIRE(std::string_view(wreck->name_source_attribute) == "NOBJNM");
  REQUIRE(wreck->active_rule_id != nullptr);
  REQUIRE(std::string_view(wreck->active_rule_id).find("wrecks") != std::string_view::npos);
  REQUIRE(wreck->active_rule_label != nullptr);
  REQUIRE(std::string_view(wreck->active_rule_label).find("WRECKS") != std::string_view::npos);
  REQUIRE(wreck->active_style_key != nullptr);
  REQUIRE(std::string_view(wreck->active_style_key) == "point/danger");
  REQUIRE(wreck->text_style_key != nullptr);
  REQUIRE(std::string_view(wreck->text_style_key) == "text/default");
  REQUIRE(wreck->suppressed == 0U);
  REQUIRE(wreck->hit_distance_m == Catch::Approx(0.0));

  chart_view_runtime_destroy(rt);
}

TEST_CASE("runtime describes a queried feature with active rule explanation through the C API", "[runtime][api][query][describe]")
{
  chart_view_runtime_t *rt = nullptr;
  REQUIRE(chart_view_runtime_create(&rt) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_initialize(rt) == chart_view_status_ok);

  auto senc = buildQueryableSenc();
  REQUIRE(chart_view_runtime_load_senc(rt, senc.data(), static_cast<std::uint32_t>(senc.size()))
          == chart_view_status_ok);

  chart_view_s52_mariner_settings_t settings{};
  REQUIRE(chart_view_runtime_get_s52_mariner_settings(rt, &settings) == chart_view_status_ok);
  settings.show_text = 1U;
  settings.display_category = chart_view_s52_display_all;
  REQUIRE(chart_view_runtime_set_s52_mariner_settings(rt, &settings) == chart_view_status_ok);

  chart_view_feature_query_t query{};
  query.lon = 0.0;
  query.lat = 51.0;
  query.tolerance_m = 40.0;
  query.max_results = 4U;

  std::array<chart_view_feature_summary_t, 4> summaries{};
  std::uint32_t resultCount = static_cast<std::uint32_t>(summaries.size());
  REQUIRE(chart_view_runtime_query_features_at_point(rt, &query, summaries.data(), &resultCount)
          == chart_view_status_ok);
  REQUIRE(resultCount >= 1U);

  chart_view_feature_summary_t described{};
  REQUIRE(chart_view_runtime_describe_feature(rt, summaries[0].runtime_feature_token, &described)
          == chart_view_status_ok);
  REQUIRE(described.runtime_feature_token == summaries[0].runtime_feature_token);
  REQUIRE(described.object_acronym != nullptr);
  REQUIRE(described.active_rule_id != nullptr);
  REQUIRE(described.active_rule_label != nullptr);
  REQUIRE(described.active_style_key != nullptr);
  REQUIRE(described.min_lon <= described.max_lon);
  REQUIRE(described.min_lat <= described.max_lat);

  REQUIRE(chart_view_runtime_describe_feature(rt, 0U, &described) == chart_view_status_invalid_argument);

  chart_view_runtime_destroy(rt);
}

TEST_CASE("full render frame pipeline through C API", "[runtime][api]")
{
  chart_view_runtime_t *rt = nullptr;
  REQUIRE(chart_view_runtime_create(&rt) == chart_view_status_ok);
  REQUIRE(chart_view_runtime_initialize(rt) == chart_view_status_ok);

  // Set viewport
  chart_view_viewport_t vp{};
  vp.center_lon = 0.0;
  vp.center_lat = 51.0;
  vp.scale_denominator = 500000.0;
  vp.pixel_width = 800;
  vp.pixel_height = 600;
  REQUIRE(chart_view_runtime_set_viewport(rt, &vp) == chart_view_status_ok);

  // Load SENC
  auto senc = buildTestSenc();
  REQUIRE(chart_view_runtime_load_senc(rt, senc.data(),
    static_cast<std::uint32_t>(senc.size())) == chart_view_status_ok);

  // Render frame
  chart_view_render_frame_result_t result{};
  REQUIRE(chart_view_runtime_render_frame(rt, &result) == chart_view_status_ok);

  // We loaded 2 features (point + line), both should be visible
  REQUIRE(result.points_rendered >= 1);
  REQUIRE(result.lines_rendered >= 1);
  REQUIRE(result.total_vertices >= 3);

  chart_view_frame_buffer_info_t frameInfo{};
  REQUIRE(chart_view_runtime_get_frame_buffer_info(rt, &frameInfo) == chart_view_status_ok);
  REQUIRE(frameInfo.pixel_width == static_cast<std::uint32_t>(vp.pixel_width));
  REQUIRE(frameInfo.pixel_height == static_cast<std::uint32_t>(vp.pixel_height));
  REQUIRE(frameInfo.rgba_size_bytes == static_cast<std::uint32_t>(vp.pixel_width * vp.pixel_height * 4));

  std::vector<std::uint8_t> rgba(frameInfo.rgba_size_bytes, 0U);
  REQUIRE(chart_view_runtime_copy_frame_rgba(rt, rgba.data(),
    static_cast<std::uint32_t>(rgba.size())) == chart_view_status_ok);

  const std::array<std::uint8_t, 4> background{230U, 230U, 217U, 255U};
  const auto nonBackgroundPixel = std::ranges::any_of(
    std::views::iota(std::size_t{0}, rgba.size() / 4U),
    [&](std::size_t pixelIndex) {
      const auto offset = pixelIndex * 4U;
      return rgba[offset + 0] != background[0]
          || rgba[offset + 1] != background[1]
          || rgba[offset + 2] != background[2]
          || rgba[offset + 3] != background[3];
    });
  REQUIRE(nonBackgroundPixel);

  chart_view_runtime_shutdown(rt);
  chart_view_runtime_destroy(rt);
}
