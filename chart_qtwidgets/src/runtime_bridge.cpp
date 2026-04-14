#include "chart_qtwidgets/runtime_bridge.hpp"

#include <utility>

namespace chartsys::qtwidgets {

RuntimeBridge::RuntimeBridge() {
  chart_runtime_create_info create_info{};
  runtime_ = chart_runtime_create(&create_info);
  (void)chart_runtime_initialize(runtime_);
}

RuntimeBridge::~RuntimeBridge() {
  if(runtime_ != nullptr) {
    (void)chart_runtime_shutdown(runtime_);
    chart_runtime_destroy(runtime_);
  }
}

RuntimeBridge::RuntimeBridge(RuntimeBridge&& other) noexcept : runtime_(std::exchange(other.runtime_, nullptr)) {}

auto RuntimeBridge::operator=(RuntimeBridge&& other) noexcept -> RuntimeBridge& {
  if(this == &other) {
    return *this;
  }

  if(runtime_ != nullptr) {
    (void)chart_runtime_shutdown(runtime_);
    chart_runtime_destroy(runtime_);
  }

  runtime_ = std::exchange(other.runtime_, nullptr);
  return *this;
}

auto RuntimeBridge::setViewport(int width, int height, float dpr) -> void {
  chart_runtime_viewport viewport{
    static_cast<uint32_t>(width > 0 ? width : 0),
    static_cast<uint32_t>(height > 0 ? height : 0),
    dpr,
  };
  (void)chart_runtime_set_viewport(runtime_, &viewport);
}

} // namespace chartsys::qtwidgets
