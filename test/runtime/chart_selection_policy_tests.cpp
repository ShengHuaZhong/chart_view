#include <catch2/catch_test_macros.hpp>

#include "catalog/chart_selection_policy.hpp"

#include <string>
#include <utility>
#include <vector>

namespace {

chart_view::runtime::catalog::ChartCatalogEntry makeEntry(
  std::string id,
  chart_view_chart_source_type_t sourceType,
  double nativeScale,
  std::uint32_t usageBand)
{
  chart_view::runtime::catalog::ChartCatalogEntry entry;
  entry.id = std::move(id);
  entry.sourceType = sourceType;
  entry.nativeScale = nativeScale;
  entry.usageBand = usageBand;
  entry.sencPath = entry.id + ".senc";
  entry.extent = {120.0, 30.0, 121.0, 31.0};
  return entry;
}

std::vector<const chart_view::runtime::catalog::ChartCatalogEntry *> asPointers(
  const std::vector<chart_view::runtime::catalog::ChartCatalogEntry> &entries)
{
  std::vector<const chart_view::runtime::catalog::ChartCatalogEntry *> pointers;
  pointers.reserve(entries.size());
  for(const auto &entry : entries) {
    pointers.push_back(&entry);
  }
  return pointers;
}

}// namespace

TEST_CASE("ChartSelectionPolicy prefers native scale closest to viewport scale", "[selection]")
{
  std::vector<chart_view::runtime::catalog::ChartCatalogEntry> candidates;
  candidates.push_back(makeEntry("harbor", chart_view_chart_source_s57, 20000.0, 6));
  candidates.push_back(makeEntry("approach", chart_view_chart_source_s57, 80000.0, 5));
  candidates.push_back(makeEntry("coastal", chart_view_chart_source_s57, 150000.0, 4));

  chart_view::runtime::catalog::ChartSelectionPolicy policy;
  const auto ranked = policy.rankCandidates(asPointers(candidates), 90000.0);

  REQUIRE(ranked.size() == 3);
  REQUIRE(ranked[0]->id == "approach");
  REQUIRE(ranked[1]->id == "coastal");
  REQUIRE(ranked[2]->id == "harbor");
}

TEST_CASE("ChartSelectionPolicy uses usage band as tie breaker", "[selection]")
{
  std::vector<chart_view::runtime::catalog::ChartCatalogEntry> candidates;
  candidates.push_back(makeEntry("usage5", chart_view_chart_source_s57, 100000.0, 5));
  candidates.push_back(makeEntry("usage3", chart_view_chart_source_s57, 100000.0, 3));

  chart_view::runtime::catalog::ChartSelectionPolicy policy;
  const auto ranked = policy.rankCandidates(asPointers(candidates), 90000.0);

  REQUIRE(ranked.size() == 2);
  REQUIRE(ranked[0]->id == "usage5");
  REQUIRE(ranked[1]->id == "usage3");
}

TEST_CASE("ChartSelectionPolicy uses source priority after scale and usage", "[selection]")
{
  std::vector<chart_view::runtime::catalog::ChartCatalogEntry> candidates;
  candidates.push_back(makeEntry("cm93", chart_view_chart_source_cm93, 100000.0, 5));
  candidates.push_back(makeEntry("s57", chart_view_chart_source_s57, 100000.0, 5));
  candidates.push_back(makeEntry("s101", chart_view_chart_source_s101, 100000.0, 5));

  chart_view::runtime::catalog::ChartSelectionPolicy policy;
  const auto ranked = policy.rankCandidates(asPointers(candidates), 90000.0);

  REQUIRE(ranked.size() == 3);
  REQUIRE(ranked[0]->id == "s101");
  REQUIRE(ranked[1]->id == "s57");
  REQUIRE(ranked[2]->id == "cm93");
}

TEST_CASE("ChartSelectionPolicy falls back to stable id ordering", "[selection]")
{
  std::vector<chart_view::runtime::catalog::ChartCatalogEntry> candidates;
  candidates.push_back(makeEntry("bravo", chart_view_chart_source_s57, 100000.0, 5));
  candidates.push_back(makeEntry("alpha", chart_view_chart_source_s57, 100000.0, 5));

  chart_view::runtime::catalog::ChartSelectionPolicy policy;
  const auto ranked = policy.rankCandidates(asPointers(candidates), 90000.0);

  REQUIRE(ranked.size() == 2);
  REQUIRE(ranked[0]->id == "alpha");
  REQUIRE(ranked[1]->id == "bravo");
}
