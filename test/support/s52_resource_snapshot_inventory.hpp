#ifndef CHART_VIEW_TEST_SUPPORT_S52_RESOURCE_SNAPSHOT_INVENTORY_HPP
#define CHART_VIEW_TEST_SUPPORT_S52_RESOURCE_SNAPSHOT_INVENTORY_HPP

#include <QJsonDocument>

#include <filesystem>

namespace chart_view::test_support {

struct S52ResourceSnapshotInventoryResult
{
  QJsonDocument document;
  int lookupRowsTotal{0};
  int supportedRows{0};
  int partialRows{0};
  int unsupportedRows{0};
};

[[nodiscard]] S52ResourceSnapshotInventoryResult buildS52ResourceSnapshotInventory(
  const std::filesystem::path &projectSourceDir);

} // namespace chart_view::test_support

#endif
