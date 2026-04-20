#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

std::filesystem::path projectRoot()
{
  return std::filesystem::path(CHART_VIEW_PROJECT_SOURCE_DIR);
}

std::filesystem::path officialDaiPath()
{
  return projectRoot() / "docs" / "reference_local" / "PresLib_e4.0.0.dai";
}

std::filesystem::path fallbackSnapshotPath()
{
  return projectRoot() / "vendor" / "opencpn_s57data" / "Release_5.14.0" / "s57data"
       / "chartsymbols.xml";
}

std::string readUppercase(const std::filesystem::path &path)
{
  std::ifstream input(path, std::ios::binary);
  REQUIRE(input.good());

  std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
    return static_cast<char>(std::toupper(ch));
  });
  return text;
}

void requireAuditInputs()
{
  if(!std::filesystem::exists(officialDaiPath())) {
    SKIP("local official e4.0.0 DAI reference is missing");
  }

  REQUIRE(std::filesystem::exists(fallbackSnapshotPath()));
}

struct AuditRoute
{
  const char *assetId;
  const char *bucket;
  const char *notes;
  const char *officialNeighbor;
};

const std::vector<AuditRoute> &auditRoutes()
{
  static const std::vector<AuditRoute> routes = {
    {"BOYSPH79",
     "alias_provenance_audit",
     "present in fallback snapshot only; do not promote to an official e4.0.0 asset without a proven alias",
     "BOYSPH01"},
    {"TOPSHP33",
     "alias_provenance_audit",
     "present in fallback snapshot only; nearby official e4.0.0 topmark symbol exists but alias is unproven",
     "TOPMAR33"},
    {"VEHTRF01",
     "inland_current",
     "kept on the inland-current route and must not flow back into the ordinary maritime mainline",
     ""},
    {"BOYLAT52", "legacy_inland", "deferred legacy inland symbol family", ""},
    {"BOYLAT53", "legacy_inland", "deferred legacy inland symbol family", ""},
    {"BOYLAT54", "legacy_inland", "deferred legacy inland symbol family", ""},
    {"BOYLAT55", "legacy_inland", "deferred legacy inland symbol family", ""},
    {"BOYLAT56", "legacy_inland", "deferred legacy inland symbol family", ""},
    {"BOYSPP50", "legacy_inland", "deferred legacy inland symbol family", ""},
    {"BCNCON81",
     "residual_compatibility",
     "fallback compatibility residue with no direct official e4.0.0 DAI definition",
     ""},
    {"DANGER53",
     "residual_compatibility",
     "fallback compatibility residue with no direct official e4.0.0 DAI definition",
     ""},
    {"BOYSPR02",
     "residual_compatibility",
     "fallback compatibility residue with no direct official e4.0.0 DAI definition",
     ""},
    {"BOYSPR03",
     "residual_compatibility",
     "fallback compatibility residue with no direct official e4.0.0 DAI definition",
     ""},
  };
  return routes;
}

} // namespace

TEST_CASE("e4.0.0 alias audit keeps non-direct asset IDs out of the official DAI path",
          "[portrayal][s52][official][e400][audit]")
{
  requireAuditInputs();

  const auto officialDai = readUppercase(officialDaiPath());
  const auto fallbackSnapshot = readUppercase(fallbackSnapshotPath());

  for(const auto &route : auditRoutes()) {
    INFO(route.assetId);
    CHECK(officialDai.find(route.assetId) == std::string::npos);
    CHECK(fallbackSnapshot.find(route.assetId) != std::string::npos);
  }

  CHECK(officialDai.find("BOYSPH01") != std::string::npos);
  CHECK(officialDai.find("TOPMAR33") != std::string::npos);
}

TEST_CASE("e4.0.0 alias audit preserves explicit routing buckets",
          "[portrayal][s52][official][e400][audit][routing]")
{
  const auto &routes = auditRoutes();

  const auto countBucket = [&routes](const char *bucket) {
    return std::count_if(routes.begin(), routes.end(), [bucket](const auto &route) {
      return std::string_view(route.bucket) == bucket;
    });
  };

  REQUIRE(countBucket("alias_provenance_audit") == 2);
  REQUIRE(countBucket("inland_current") == 1);
  REQUIRE(countBucket("legacy_inland") == 6);
  REQUIRE(countBucket("residual_compatibility") == 4);

  const auto boysphAudit = std::find_if(routes.begin(), routes.end(), [](const auto &route) {
    return std::string_view(route.assetId) == "BOYSPH79";
  });
  REQUIRE(boysphAudit != routes.end());
  CHECK(std::string_view(boysphAudit->officialNeighbor) == "BOYSPH01");

  const auto topshpAudit = std::find_if(routes.begin(), routes.end(), [](const auto &route) {
    return std::string_view(route.assetId) == "TOPSHP33";
  });
  REQUIRE(topshpAudit != routes.end());
  CHECK(std::string_view(topshpAudit->officialNeighbor) == "TOPMAR33");
}
