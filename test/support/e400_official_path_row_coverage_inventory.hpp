#ifndef CHART_VIEW_TEST_SUPPORT_E400_OFFICIAL_PATH_ROW_COVERAGE_INVENTORY_HPP
#define CHART_VIEW_TEST_SUPPORT_E400_OFFICIAL_PATH_ROW_COVERAGE_INVENTORY_HPP

#include <QJsonDocument>

#include <filesystem>

namespace chart_view::test_support {

struct E400OfficialPathRowCoverageInventoryResult
{
  QJsonDocument document;
  int lookupRowsTotal{0};
  int supportedRows{0};
  int partialRows{0};
  int unsupportedRows{0};
};

[[nodiscard]] E400OfficialPathRowCoverageInventoryResult buildE400OfficialPathRowCoverageInventory(
  const std::filesystem::path &projectSourceDir);

} // namespace chart_view::test_support

#endif
