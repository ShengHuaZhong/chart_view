#include "render_scheduler.hpp"

namespace chart_view::runtime {

void RenderScheduler::submitSnapshot(std::shared_ptr<const SceneSnapshot> snapshot)
{
  m_latest = std::move(snapshot);
  ++m_frameCount;
}

}// namespace chart_view::runtime
