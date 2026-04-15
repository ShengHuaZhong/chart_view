#include "runtime_context.hpp"
#include "scene_builder_from_senc.hpp"
#include "cm93/cm93_reader.hpp"
#include "s101/s101_reader.hpp"
#include "s57/s57_reader.hpp"
#include "senc/senc_reader.hpp"
#include "senc/senc_writer.hpp"

#include <internal_use_only/config.hpp>

#include <cctype>
#include <filesystem>
#include <string>

namespace {
constexpr float kFrameClearR = 230.0F / 255.0F;
constexpr float kFrameClearG = 230.0F / 255.0F;
constexpr float kFrameClearB = 217.0F / 255.0F;
constexpr float kFrameClearA = 1.0F;

constexpr std::uint32_t buildFeatureFlags() noexcept
{
  std::uint32_t flags = 0;

  if(chart_view::build::enable_qt_rhi) {
    flags |= CHART_VIEW_FEATURE_QT_RHI;
  }

  if(chart_view::build::enable_s57) {
    flags |= CHART_VIEW_FEATURE_S57;
  }

  if(chart_view::build::enable_cm93) {
    flags |= CHART_VIEW_FEATURE_CM93;
  }

  if(chart_view::build::enable_s101) {
    flags |= CHART_VIEW_FEATURE_S101;
  }

  return flags;
}

std::string toLowerAscii(std::string value)
{
  for(char &ch : value) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return value;
}

chart_view_chart_source_type_t detectSourceTypeFromPath(std::string_view path)
{
  if(path.empty()) {
    return chart_view_chart_source_unknown;
  }

  std::error_code ec;
  if(std::filesystem::is_directory(path, ec)) {
    return chart_view_chart_source_cm93;
  }

  auto ext = toLowerAscii(std::filesystem::path(path).extension().string());
  if(!ext.empty() && ext.front() == '.') {
    ext.erase(ext.begin());
  }

  if(ext == "c" || ext == "cm93") {
    return chart_view_chart_source_cm93;
  }

  if(ext == "s101" || ext == "101" || ext == "gml" || ext == "xml") {
    return chart_view_chart_source_s101;
  }

  if(ext == "000" || ext == "001" || ext == "002" || ext == "s57") {
    return chart_view_chart_source_s57;
  }

  return chart_view_chart_source_unknown;
}

void setViewportFromDataset(chart_view::runtime::ViewportState &viewport,
                            const chart_view::runtime::chart_data::FeatureChartDataset &dataset)
{
  auto vp = viewport.viewport();

  if(vp.pixel_width <= 0) {
    vp.pixel_width = 1280;
  }
  if(vp.pixel_height <= 0) {
    vp.pixel_height = 720;
  }
  if(vp.scale_denominator <= 0.0) {
    vp.scale_denominator = 100000.0;
  }

  const auto &extent = dataset.meta().extent;
  if(extent.isValid()) {
    vp.center_lon = (extent.minLon + extent.maxLon) * 0.5;
    vp.center_lat = (extent.minLat + extent.maxLat) * 0.5;
  }

  viewport.set(vp);
}

bool loadSourceDataset(const std::string &path,
                       chart_view_chart_source_type_t sourceType,
                       chart_view::runtime::chart_data::FeatureChartDataset &out)
{
  switch(sourceType) {
  case chart_view_chart_source_s57: {
    chart_view::runtime::s57::S57Reader reader;
    auto result = reader.read(path);
    if(!result.ok) {
      return false;
    }
    out = std::move(result.dataset);
    return true;
  }
  case chart_view_chart_source_cm93: {
    chart_view::runtime::cm93::Cm93Reader reader;
    std::error_code ec;
    auto result = std::filesystem::is_directory(path, ec) ? reader.readFirstCell(path) : reader.read(path);
    if(!result.ok) {
      std::fprintf(stderr, "cm93_read_failed: %s\n", result.error.c_str());
      return false;
    }
    out = std::move(result.dataset);
    return true;
  }
  case chart_view_chart_source_s101: {
    chart_view::runtime::s101::S101Reader reader;
    auto result = reader.read(path);
    if(!result.ok) {
      return false;
    }
    out = std::move(result.dataset);
    return true;
  }
  case chart_view_chart_source_unknown:
  default:
    return false;
  }
}
}// namespace

namespace chart_view::runtime {

RuntimeContext::RuntimeContext()
  : m_info{
    CHART_VIEW_RUNTIME_ABI_VERSION,
    chart_view::build::project_name.data(),
    chart_view::build::project_version.data(),
    chart_view::build::git_sha.data(),
    buildFeatureFlags()
  }
{
}

RuntimeContext::~RuntimeContext()
{
  if(m_state == RuntimeState::kInitialized) {
    shutdown();
  }
}

chart_view_status_t RuntimeContext::initialize()
{
  if(m_state == RuntimeState::kInitialized) {
    return chart_view_status_already_initialized;
  }

  m_state = RuntimeState::kInitialized;
  return chart_view_status_ok;
}

chart_view_status_t RuntimeContext::shutdown()
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  m_dataset.reset();
  m_state = RuntimeState::kShutDown;
  return chart_view_status_ok;
}

chart_view_status_t RuntimeContext::ensureRenderTarget()
{
  if(!m_viewport.isValid()) {
    return chart_view_status_invalid_argument;
  }

  const auto &vp = m_viewport.viewport();
  return m_renderBackend.ensureSurfaceSize(vp.pixel_width, vp.pixel_height);
}

chart_view_status_t RuntimeContext::setViewport(const chart_view_viewport_t &vp)
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  m_viewport.set(vp);
  if(m_viewport.isValid()) {
    const auto status = ensureRenderTarget();
    if(status != chart_view_status_ok) {
      return status;
    }

    const auto clearStatus = m_renderBackend.renderClearFrame(kFrameClearR, kFrameClearG, kFrameClearB, kFrameClearA);
    if(clearStatus != chart_view_status_ok) {
      return clearStatus;
    }
  }

  return chart_view_status_ok;
}

chart_view_status_t RuntimeContext::loadSenc(std::span<const std::uint8_t> data)
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  if(data.empty()) {
    return chart_view_status_invalid_argument;
  }

  senc::SencReader reader;
  auto result = reader.read(data);
  if(!result.ok) {
    return chart_view_status_invalid_format;
  }

  m_dataset = std::make_unique<chart_data::FeatureChartDataset>(std::move(result.dataset));
  return chart_view_status_ok;
}

chart_view_status_t RuntimeContext::openChartFile(std::string_view path,
                                                  chart_view_chart_source_type_t sourceType)
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  if(path.empty()) {
    return chart_view_status_invalid_argument;
  }

  const auto resolvedType = sourceType == chart_view_chart_source_unknown
                              ? detectSourceTypeFromPath(path)
                              : sourceType;
  if(resolvedType == chart_view_chart_source_unknown) {
    return chart_view_status_invalid_argument;
  }

  chart_data::FeatureChartDataset dataset;
  if(!loadSourceDataset(std::string(path), resolvedType, dataset)) {
    return chart_view_status_invalid_format;
  }

  senc::SencWriter writer;
  auto sencBlob = writer.write(dataset);
  if(sencBlob.empty()) {
    return chart_view_status_invalid_format;
  }

  setViewportFromDataset(m_viewport, dataset);

  return loadSenc(std::span<const std::uint8_t>(sencBlob.data(), sencBlob.size()));
}

void RuntimeContext::getLoadedChartInfo(chart_view_loaded_chart_info_t &out) const
{
  out = {};

  if(!m_dataset) {
    out.source_type = chart_view_chart_source_unknown;
    return;
  }

  const auto &meta = m_dataset->meta();
  out.source_type = meta.sourceType;
  out.feature_count = static_cast<std::uint32_t>(m_dataset->featureCount());
  out.min_lon = meta.extent.minLon;
  out.min_lat = meta.extent.minLat;
  out.max_lon = meta.extent.maxLon;
  out.max_lat = meta.extent.maxLat;
}

void RuntimeContext::getViewport(chart_view_viewport_t &out) const
{
  out = m_viewport.viewport();
}

chart_view_status_t RuntimeContext::renderFrame(chart_view_render_frame_result_t &out)
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  out = {};

  if(!m_dataset || !m_viewport.isValid()) {
    return chart_view_status_ok;
  }

  const auto targetStatus = ensureRenderTarget();
  if(targetStatus != chart_view_status_ok) {
    return targetStatus;
  }

  SceneBuilderFromSenc builder;
  auto snap = builder.build(*m_dataset, m_viewport);

  if(!snap || snap->empty()) {
    (void)m_renderBackend.renderClearFrame(kFrameClearR, kFrameClearG, kFrameClearB, kFrameClearA);
    return chart_view_status_ok;
  }

  m_scheduler.submitSnapshot(snap);

  const auto renderResult = m_renderer.render(*snap, *m_dataset, m_renderBackend);
  if(renderResult.status != chart_view_status_ok) {
    return renderResult.status;
  }

  out.points_rendered = renderResult.pointsRendered;
  out.lines_rendered = renderResult.linesRendered;
  out.areas_rendered = renderResult.areasRendered;
  out.total_vertices = renderResult.totalVertices;

  return chart_view_status_ok;
}

void RuntimeContext::getFrameBufferInfo(chart_view_frame_buffer_info_t &out) const
{
  out = {};
  if(!m_renderBackend.isInitialized()) {
    return;
  }

  out.pixel_width = static_cast<std::uint32_t>(m_renderBackend.surfaceWidth());
  out.pixel_height = static_cast<std::uint32_t>(m_renderBackend.surfaceHeight());
  out.stride_bytes = m_renderBackend.strideBytes();
  out.rgba_size_bytes = static_cast<std::uint32_t>(m_renderBackend.frameByteSize());
}

chart_view_status_t RuntimeContext::copyFrameRgba(std::span<std::uint8_t> dst) const
{
  if(m_state != RuntimeState::kInitialized) {
    return chart_view_status_not_initialized;
  }

  return m_renderBackend.copyFrameRgba(dst);
}

}// namespace chart_view::runtime
