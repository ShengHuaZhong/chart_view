#include <catch2/catch_test_macros.hpp>

// Internal runtime headers (via target_include_directories)
#include "rhi_render_backend.hpp"

#include <QGuiApplication>

namespace {
// QRhi requires a QGuiApplication instance.
// Create one if not already running.
struct AppGuard
{
  static int argc;
  static char *argv[];
  QGuiApplication app{argc, argv};
};
int AppGuard::argc = 1;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
char *AppGuard::argv[] = {const_cast<char *>("rhi_render_tests")};
}// namespace

TEST_CASE("RhiRenderBackend starts uninitialized", "[runtime][rhi]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;
  REQUIRE_FALSE(backend.isInitialized());
  REQUIRE(backend.rhi() == nullptr);
}

TEST_CASE("RhiRenderBackend::initialize with Null backend", "[runtime][rhi]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;

  REQUIRE(backend.initialize(640, 480) == chart_view_status_ok);
  REQUIRE(backend.isInitialized());
  REQUIRE(backend.rhi() != nullptr);

  // Double init is rejected.
  REQUIRE(backend.initialize(640, 480) == chart_view_status_already_initialized);
}

TEST_CASE("RhiRenderBackend::initialize rejects invalid size", "[runtime][rhi]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;

  REQUIRE(backend.initialize(0, 480) == chart_view_status_invalid_argument);
  REQUIRE(backend.initialize(640, 0) == chart_view_status_invalid_argument);
  REQUIRE(backend.initialize(-1, 480) == chart_view_status_invalid_argument);
  REQUIRE_FALSE(backend.isInitialized());
}

TEST_CASE("RhiRenderBackend::renderClearFrame executes on Null backend", "[runtime][rhi]")
{
  AppGuard guard;
  chart_view::runtime::RhiRenderBackend backend;

  // Cannot render before init.
  REQUIRE(backend.renderClearFrame(0.0F, 0.0F, 0.5F, 1.0F) == chart_view_status_not_initialized);

  REQUIRE(backend.initialize(320, 240) == chart_view_status_ok);

  // Clear pass with Null backend should succeed.
  REQUIRE(backend.renderClearFrame(0.0F, 0.0F, 0.5F, 1.0F) == chart_view_status_ok);
  // Multiple frames should succeed.
  REQUIRE(backend.renderClearFrame(1.0F, 0.0F, 0.0F, 1.0F) == chart_view_status_ok);
}
