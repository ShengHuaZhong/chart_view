#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S52_DISPLAY_SETTINGS_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S52_DISPLAY_SETTINGS_HPP

namespace chart_view::runtime::portrayal {

enum class S52ColorScheme
{
  kDay,
  kDusk,
  kNight,
};

enum class S52PointSymbolMode
{
  kTraditional,
  kSimplified,
};

enum class S52DisplayCategory
{
  kDisplayBase,
  kStandard,
  kAll,
};

struct S52DisplaySettings
{
  S52ColorScheme colorScheme{S52ColorScheme::kDay};
  S52DisplayCategory displayCategory{S52DisplayCategory::kStandard};
  S52PointSymbolMode pointSymbolMode{S52PointSymbolMode::kTraditional};
  bool showSoundings{true};
  bool showTextLabels{true};
  bool twoShades{false};
  double safetyContourMeters{30.0};
  double safetyDepthMeters{30.0};
  double shallowContourMeters{2.0};
  double deepContourMeters{30.0};
  bool shallowPattern{true};
  bool fullSectorLights{false};
  bool symbolizedBoundaries{true};
  bool honorScamin{true};
};

}// namespace chart_view::runtime::portrayal

#endif
