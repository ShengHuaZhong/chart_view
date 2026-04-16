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
#include <string_view>
#include <vector>

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
