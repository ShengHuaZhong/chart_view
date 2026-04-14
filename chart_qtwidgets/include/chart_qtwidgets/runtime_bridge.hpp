#pragma once

#include "runtime/chart_runtime_c_api.h"

namespace chartsys::qtwidgets {

class RuntimeBridge {
public:
  RuntimeBridge();
  ~RuntimeBridge();

  RuntimeBridge(const RuntimeBridge&) = delete;
  auto operator=(const RuntimeBridge&) -> RuntimeBridge& = delete;

  RuntimeBridge(RuntimeBridge&&) noexcept;
  auto operator=(RuntimeBridge&&) noexcept -> RuntimeBridge&;

  auto setViewport(int width, int height, float dpr) -> void;

private:
  chart_runtime_handle runtime_{nullptr};
};

} // namespace chartsys::qtwidgets
