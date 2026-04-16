#include <catch2/catch_test_macros.hpp>

#include "portrayal/s52_presentation_assets.hpp"

TEST_CASE("S52PresentationAssets exposes baseline palette and asset queries", "[portrayal][s52][assets]")
{
  chart_view::runtime::portrayal::S52PresentationAssets assets;
  const chart_view::runtime::SurfaceColor expectedDepthColor{162U, 201U, 229U, 255U};

  const auto *depthColor = assets.findColor("depdw");
  REQUIRE(depthColor != nullptr);
  REQUIRE(depthColor->token == "DEPDW");
  REQUIRE(depthColor->color == expectedDepthColor);

  const auto *buoySymbol = assets.findPointSymbol("boyspp01");
  REQUIRE(buoySymbol != nullptr);
  REQUIRE(buoySymbol->assetId == "BOYSPP01");
  REQUIRE(buoySymbol->colorToken == "CHYLW");
  REQUIRE(buoySymbol->radius == 4);

  const auto *channelLine = assets.findLineStyle("fairwy01");
  REQUIRE(channelLine != nullptr);
  REQUIRE(channelLine->assetId == "FAIRWY01");
  REQUIRE(channelLine->colorToken == "CHGRN");
  REQUIRE(channelLine->thickness == 2);

  const auto *restrictedArea = assets.findAreaPattern("resare01");
  REQUIRE(restrictedArea != nullptr);
  REQUIRE(restrictedArea->assetId == "RESARE01");
  REQUIRE(restrictedArea->fillColorToken == "RESDR");
  REQUIRE(restrictedArea->outlineColorToken == "CHRED");
  REQUIRE(restrictedArea->holeFillColorToken == "NODTA");
  REQUIRE(restrictedArea->fillAlpha == 220U);

  REQUIRE(assets.findColor("missing") == nullptr);
  REQUIRE(assets.findPointSymbol("missing") == nullptr);
  REQUIRE(assets.findLineStyle("missing") == nullptr);
  REQUIRE(assets.findAreaPattern("missing") == nullptr);
}

TEST_CASE("S52PresentationAssets resolves colors with fallback", "[portrayal][s52][assets]")
{
  chart_view::runtime::portrayal::S52PresentationAssets assets;

  const chart_view::runtime::SurfaceColor fallback{1U, 2U, 3U, 4U};
  const chart_view::runtime::SurfaceColor expectedBlack{24U, 38U, 55U, 255U};
  REQUIRE(assets.resolveColor("chblk", fallback) == expectedBlack);
  REQUIRE(assets.resolveColor("unknown-token", fallback) == fallback);
}
