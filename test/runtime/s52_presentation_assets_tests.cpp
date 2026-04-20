#include <catch2/catch_test_macros.hpp>

#include "portrayal/s52_presentation_assets.hpp"

TEST_CASE("S52PresentationAssets exposes baseline palette and asset queries", "[portrayal][s52][assets]")
{
  chart_view::runtime::portrayal::S52PresentationAssets assets;
  using chart_view::runtime::portrayal::S52PaletteId;

  const auto *depthColor = assets.findColor("depdw");
  REQUIRE(depthColor != nullptr);
  REQUIRE(depthColor->token == "DEPDW");
  REQUIRE(depthColor->palette == S52PaletteId::kDay);
  REQUIRE(depthColor->tableName == "DAY");
  REQUIRE(depthColor->color[3] == 255U);

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
  REQUIRE(anchorageBoundary->hpgl.starts_with("SPA;SW1;PU306,812;PD906,812;"));

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
  const chart_view::runtime::SurfaceColor expectedBlack{0U, 0U, 0U, 255U};
  REQUIRE(assets.resolveColor("chblk", fallback) == expectedBlack);
  REQUIRE(assets.resolveColor("unknown-token", fallback) == fallback);
}

TEST_CASE("S52PresentationAssets selects palette-specific color tables", "[portrayal][s52][assets][palette]")
{
  using chart_view::runtime::portrayal::S52PaletteId;
  using chart_view::runtime::portrayal::S52PresentationAssets;

  const S52PresentationAssets dayAssets;
  const S52PresentationAssets duskAssets(S52PaletteId::kDusk);
  const S52PresentationAssets nightAssets(S52PaletteId::kNight);

  const auto *dayBackground = dayAssets.findColor("NODTA");
  const auto *duskBackground = duskAssets.findColor("NODTA");
  const auto *nightBackground = nightAssets.findColor("NODTA");

  REQUIRE(dayBackground != nullptr);
  REQUIRE(duskBackground != nullptr);
  REQUIRE(nightBackground != nullptr);
  REQUIRE(dayBackground->palette == S52PaletteId::kDay);
  REQUIRE(duskBackground->palette == S52PaletteId::kDusk);
  REQUIRE(nightBackground->palette == S52PaletteId::kNight);
  REQUIRE(dayBackground->color != duskBackground->color);
  REQUIRE(dayBackground->color != nightBackground->color);
  REQUIRE(duskBackground->color != nightBackground->color);
}

TEST_CASE("S52PresentationAssets exposes official e4.0.0 wave-1 static assets",
          "[portrayal][s52][assets][official][wave1]")
{
  chart_view::runtime::portrayal::S52PresentationAssets assets;

  const auto *floatingHazard = assets.findPointSymbol("FLTHAZ02");
  REQUIRE(floatingHazard != nullptr);
  REQUIRE(floatingHazard->assetId == "FLTHAZ02");
  REQUIRE(floatingHazard->colorToken == "ACHMGD");
  REQUIRE(floatingHazard->sourceRcid == "SY01597");
  REQUIRE(floatingHazard->vectorMetrics.width == 648);
  REQUIRE(floatingHazard->vectorMetrics.height == 648);
  REQUIRE(floatingHazard->vectorMetrics.pivot.valid);
  REQUIRE(floatingHazard->vectorMetrics.origin.valid);

  const auto *essaSymbol = assets.findPointSymbol("ESSARE01");
  REQUIRE(essaSymbol != nullptr);
  REQUIRE(essaSymbol->assetId == "ESSARE01");
  REQUIRE(essaSymbol->colorToken == "ACHMGF");
  REQUIRE(essaSymbol->sourceRcid == "SY01589");
  REQUIRE(essaSymbol->vectorMetrics.width == 1270);
  REQUIRE(essaSymbol->vectorMetrics.height == 500);

  const auto *pssaSymbol = assets.findPointSymbol("PSSARE01");
  REQUIRE(pssaSymbol != nullptr);
  REQUIRE(pssaSymbol->assetId == "PSSARE01");
  REQUIRE(pssaSymbol->colorToken == "ACHMGF");
  REQUIRE(pssaSymbol->sourceRcid == "SY01673");
  REQUIRE(pssaSymbol->vectorMetrics.width == 1270);
  REQUIRE(pssaSymbol->vectorMetrics.height == 500);

  const auto *essaBoundary = assets.findLineStyle("ESSARE01");
  REQUIRE(essaBoundary != nullptr);
  REQUIRE(essaBoundary->assetId == "ESSARE01");
  REQUIRE(essaBoundary->colorToken == "ACHMGF");
  REQUIRE(essaBoundary->sourceRcid == "LS01364");
  REQUIRE(essaBoundary->vectorMetrics.width == 300);
  REQUIRE(essaBoundary->vectorMetrics.height == 150);
  REQUIRE(essaBoundary->hpgl.starts_with("SPA;SW1;PU200,800"));
}
