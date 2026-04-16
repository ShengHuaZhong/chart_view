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

struct S52DisplaySettings
{
  S52ColorScheme colorScheme{S52ColorScheme::kDay};
  S52PointSymbolMode pointSymbolMode{S52PointSymbolMode::kTraditional};
  bool showSoundings{true};
  bool showTextLabels{true};
};

}// namespace chart_view::runtime::portrayal

#endif
