#include <catch2/catch_test_macros.hpp>

#include "portrayal/s52_presentation_assets.hpp"

TEST_CASE("S52PresentationAssets exposes baseline palette and asset queries", "[portrayal][s52][assets]")
{
  chart_view::runtime::portrayal::S52PresentationAssets assets;
  const chart_view::runtime::SurfaceColor expectedDepthColor{212U, 234U, 238U, 255U};

  const auto *depthColor = assets.findColor("depdw");
  REQUIRE(depthColor != nullptr);
  REQUIRE(depthColor->token == "DEPDW");
  REQUIRE(depthColor->color == expectedDepthColor);

  const auto *anchorageSymbol = assets.findPointSymbol("achare02");
  REQUIRE(anchorageSymbol != nullptr);
  REQUIRE(anchorageSymbol->assetId == "ACHARE02");
  REQUIRE(anchorageSymbol->colorToken == "ACHMGD");
  REQUIRE(anchorageSymbol->vectorMetrics.width == 402);
  REQUIRE(anchorageSymbol->vectorMetrics.pivot.valid);
  REQUIRE(anchorageSymbol->vectorMetrics.origin.valid);

  const auto *anchorageBoundary = assets.findLineStyle("achare51");
  REQUIRE(anchorageBoundary != nullptr);
  REQUIRE(anchorageBoundary->assetId == "ACHARE51");
  REQUIRE(anchorageBoundary->colorToken == "ACHMGD");
  REQUIRE(anchorageBoundary->vectorMetrics.width == 3030);
  REQUIRE(anchorageBoundary->hpgl.starts_with("SPA;SW1;PU1429,568"));

  const auto *incompleteSurvey = assets.findAreaPattern("prtsur01");
  REQUIRE(incompleteSurvey != nullptr);
  REQUIRE(incompleteSurvey->assetId == "PRTSUR01");
  REQUIRE(incompleteSurvey->fillColorToken == "ACHGRD");
  REQUIRE(incompleteSurvey->outlineColorToken == "ACHGRD");
  REQUIRE(incompleteSurvey->holeFillColorToken == "NODTA");
  REQUIRE(incompleteSurvey->fillType == "S");
  REQUIRE(incompleteSurvey->spacingToken == "C");
  REQUIRE(incompleteSurvey->vectorMetrics.width == 201);

  REQUIRE(assets.findColor("missing") == nullptr);
  REQUIRE(assets.findPointSymbol("missing") == nullptr);
  REQUIRE(assets.findLineStyle("missing") == nullptr);
  REQUIRE(assets.findAreaPattern("missing") == nullptr);
}

TEST_CASE("S52PresentationAssets resolves colors with fallback", "[portrayal][s52][assets]")
{
  chart_view::runtime::portrayal::S52PresentationAssets assets;

  const chart_view::runtime::SurfaceColor fallback{1U, 2U, 3U, 4U};
  const chart_view::runtime::SurfaceColor expectedBlack{7U, 7U, 7U, 255U};
  REQUIRE(assets.resolveColor("chblk", fallback) == expectedBlack);
  REQUIRE(assets.resolveColor("unknown-token", fallback) == fallback);
}
