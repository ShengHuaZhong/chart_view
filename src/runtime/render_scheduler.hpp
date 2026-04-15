#ifndef CHART_VIEW_RUNTIME_RENDER_SCHEDULER_HPP
#define CHART_VIEW_RUNTIME_RENDER_SCHEDULER_HPP

#include "scene_snapshot.hpp"

#include <cstdint>
#include <memory>

namespace chart_view::runtime {

// Skeleton render scheduler.
// Accepts a SceneSnapshot and tracks the latest snapshot revision.
// Will be extended with actual RHI submission in later tasks.
class RenderScheduler
{
public:
  RenderScheduler() = default;

  void submitSnapshot(std::shared_ptr<const SceneSnapshot> snapshot);

  [[nodiscard]] std::shared_ptr<const SceneSnapshot> latestSnapshot() const noexcept { return m_latest; }
  [[nodiscard]] std::uint64_t frameCount() const noexcept { return m_frameCount; }
  [[nodiscard]] bool hasPendingWork() const noexcept { return m_latest != nullptr; }

private:
  std::shared_ptr<const SceneSnapshot> m_latest;
  std::uint64_t m_frameCount{0};
};

}// namespace chart_view::runtime

#endif
