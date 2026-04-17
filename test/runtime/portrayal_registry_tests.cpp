#include <catch2/catch_test_macros.hpp>

#include "chart_data/feature.hpp"
#include "portrayal/portrayal_registry.hpp"
#include "portrayal/s52_presentation_assets.hpp"

TEST_CASE("PortrayalRegistry returns default rules when no override exists", "[portrayal][registry]")
{
  chart_view::runtime::portrayal::PortrayalRegistry registry;
  const chart_view::runtime::SurfaceColor defaultPointColor{196U, 46U, 46U, 255U};
  const chart_view::runtime::SurfaceColor defaultLineColor{24U, 38U, 55U, 255U};
  const chart_view::runtime::SurfaceColor defaultAreaFill{162U, 201U, 229U, 204U};
  const chart_view::runtime::SurfaceColor defaultAreaOutline{44U, 91U, 134U, 255U};
  const chart_view::runtime::SurfaceColor defaultHoleFill{230U, 230U, 217U, 255U};

  chart_view::runtime::chart_data::Feature feature;
  feature.classAcronym = "UNKNOWN";

  const auto &symbolRule = registry.resolveSymbolRule(feature);
  const auto &lineRule = registry.resolveLineStyleRule(feature);
  const auto &areaRule = registry.resolveAreaFillRule(feature);
  const auto &textRule = registry.resolveTextRule(feature);

  REQUIRE(symbolRule.color == defaultPointColor);
  REQUIRE(symbolRule.radius == 4);
  REQUIRE(lineRule.color == defaultLineColor);
  REQUIRE(lineRule.thickness == 2);
  REQUIRE(areaRule.fillColor == defaultAreaFill);
  REQUIRE(areaRule.outlineColor == defaultAreaOutline);
  REQUIRE(areaRule.holeFillColor == defaultHoleFill);
  REQUIRE(areaRule.outlineThickness == 1);
  REQUIRE(textRule.color == defaultLineColor);
  REQUIRE(textRule.pixelSize == 12U);
}

TEST_CASE("PortrayalRegistry resolves feature overrides case-insensitively", "[portrayal][registry]")
{
  chart_view::runtime::portrayal::PortrayalRegistry registry;
  const chart_view::runtime::SurfaceColor customPointColor{10U, 11U, 12U, 255U};
  const chart_view::runtime::SurfaceColor customLineColor{20U, 21U, 22U, 255U};
  const chart_view::runtime::SurfaceColor customAreaFill{30U, 31U, 32U, 200U};
  const chart_view::runtime::SurfaceColor customAreaOutline{40U, 41U, 42U, 255U};
  const chart_view::runtime::SurfaceColor customHoleFill{50U, 51U, 52U, 255U};
  const chart_view::runtime::SurfaceColor customTextColor{60U, 61U, 62U, 255U};

  registry.registerSymbolRule("soundg", {{10U, 11U, 12U, 255U}, 7});
  registry.registerLineStyleRule("depcnt", {{20U, 21U, 22U, 255U}, 3});
  registry.registerAreaFillRule(
    "depare",
    {{30U, 31U, 32U, 200U}, {40U, 41U, 42U, 255U}, {50U, 51U, 52U, 255U}, 2});
  registry.registerTextRule("soundg", {{60U, 61U, 62U, 255U}, 14U});

  chart_view::runtime::chart_data::Feature pointFeature;
  pointFeature.classAcronym = "SOUNDG";

  chart_view::runtime::chart_data::Feature lineFeature;
  lineFeature.classAcronym = "DEPCNT";

  chart_view::runtime::chart_data::Feature areaFeature;
  areaFeature.classAcronym = "DEPARE";

  const auto &symbolRule = registry.resolveSymbolRule(pointFeature);
  const auto &lineRule = registry.resolveLineStyleRule(lineFeature);
  const auto &areaRule = registry.resolveAreaFillRule(areaFeature);
  const auto &textRule = registry.resolveTextRule(pointFeature);

  REQUIRE(symbolRule.color == customPointColor);
  REQUIRE(symbolRule.radius == 7);
  REQUIRE(lineRule.color == customLineColor);
  REQUIRE(lineRule.thickness == 3);
  REQUIRE(areaRule.fillColor == customAreaFill);
  REQUIRE(areaRule.outlineColor == customAreaOutline);
  REQUIRE(areaRule.holeFillColor == customHoleFill);
  REQUIRE(areaRule.outlineThickness == 2);
  REQUIRE(textRule.color == customTextColor);
  REQUIRE(textRule.pixelSize == 14U);
}

TEST_CASE("PortrayalRegistry seeds baseline styles from S-52 presentation assets", "[portrayal][registry][s52]")
{
  chart_view::runtime::portrayal::S52PresentationAssets assets;
  chart_view::runtime::portrayal::PortrayalRegistry registry;
  const chart_view::runtime::SurfaceColor emptyColor{0U, 0U, 0U, 255U};
  const chart_view::runtime::portrayal::LineStyleRule channelFallback{{24U, 116U, 86U, 255U}, 2};
  const chart_view::runtime::portrayal::AreaFillRule landFallback{
    {196U, 190U, 137U, 255U},
    {110U, 96U, 52U, 255U},
    {230U, 230U, 217U, 255U},
    1};

  REQUIRE(registry.canvasBackgroundColor() == assets.resolveColor("NODTA", emptyColor));

  const auto *soundingAsset = assets.findPointSymbol("soundg01");
  REQUIRE(soundingAsset != nullptr);
  const auto soundingRule = registry.resolveSymbolRuleForStyle("point/sounding");
  REQUIRE(
    soundingRule.color
    == assets.resolveColor(soundingAsset->colorToken, registry.defaultSymbolRule().color));
  REQUIRE(soundingRule.radius == soundingAsset->radius);

  const auto channelRule = registry.resolveLineStyleRuleForStyle("line/channel");
  if(const auto *channelAsset = assets.findLineStyle("fairwy01"); channelAsset != nullptr) {
    REQUIRE(channelRule.color == assets.resolveColor(channelAsset->colorToken, emptyColor));
    REQUIRE(channelRule.thickness == channelAsset->thickness);
  } else {
    REQUIRE(channelRule.color == channelFallback.color);
    REQUIRE(channelRule.thickness == channelFallback.thickness);
  }

  const auto landRule = registry.resolveAreaFillRuleForStyle("area/land");
  if(const auto *landAsset = assets.findAreaPattern("lndare01"); landAsset != nullptr) {
    const auto expectedLandFill = assets.resolveColor(landAsset->fillColorToken, emptyColor);
    const auto expectedLandOutline = assets.resolveColor(landAsset->outlineColorToken, emptyColor);
    REQUIRE(landRule.fillColor == expectedLandFill);
    REQUIRE(landRule.outlineColor == expectedLandOutline);
    REQUIRE(landRule.holeFillColor == assets.resolveColor(landAsset->holeFillColorToken, emptyColor));
    REQUIRE(landRule.outlineThickness == landAsset->outlineThickness);
  } else {
    REQUIRE(landRule.fillColor == landFallback.fillColor);
    REQUIRE(landRule.outlineColor == landFallback.outlineColor);
    REQUIRE(landRule.holeFillColor == landFallback.holeFillColor);
    REQUIRE(landRule.outlineThickness == landFallback.outlineThickness);
  }
}
