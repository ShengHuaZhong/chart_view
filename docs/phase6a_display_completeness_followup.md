# Phase 6A Display Completeness Follow-up

## Scope

This follow-up tightened the fixed-scene Phase 6A portrayal baseline without widening the runtime ABI and without moving symbol or rule logic into the host layers.

It focused on the highest-value gaps exposed by the existing fixed scenes:

- traditional vs simplified lookup row selection for `BOYSPP`, `SOUNDG`, and `WRECKS`
- `OBJNAM` / `NOBJNM` text-attribute linkage on compiled text rules
- `SOUNDG02` / `WRECKS02` conditional explain retention
- point-symbol asset selection and point-glyph rendering
- the remaining `FAIRWY` informational geometry mismatch in the Chart 1 scene

## Changes

- `src/runtime/portrayal/s52_lookup_model.hpp`
- `src/runtime/portrayal/s52_lookup_model.cpp`
  - rank compiled lookup rows by point-symbol mode and table name so traditional scenes prefer `Paper` rows and simplified scenes prefer `Simplified` rows
- `src/runtime/portrayal/s52_instruction_string_parser.cpp`
  - preserve nested text instructions inside conditional instruction strings during offline compilation
- `src/runtime/portrayal/s52_conditional_symbology.hpp`
  - keep `SOUNDG02` / `WRECKS02` explain surfaces stable while still honoring sounding suppression
- `src/runtime/portrayal/feature_symbolizer.cpp`
  - preserve compiled text-attribute linkage for visible asset-bearing symbols even when text labels are hidden
  - fall back to source `OBJNAM` / `NOBJNM` for hidden-label scenes when the feature still renders a primary asset
- `src/runtime/point_symbol_renderer.cpp`
  - added the missing `BOYGEN03`, `BOYSPP11`, and `SOUNDG02` point glyphs used by the fixed scenes
- `src/runtime/feature_layer_renderer.hpp`
- `src/runtime/feature_layer_renderer.cpp`
  - align projected point anchors with label anchor resolution so crop capture and point-symbol placement stay consistent
- `test/runtime/chart1_s64_reference_harness_tests.cpp`
  - updated the fixed scenes so the buoy cases carry `OBJNAM`, matching the intended OpenCPN engineering expectations
- `tests/data/reference/*.reference.json`
  - refreshed the committed Phase 6A fixed-scene reference observations

## Result

The fixed-scene Phase 6A regression set now closes the previously visible deltas for:

- `BOYSPP` table selection and `primaryAssetId`
- `SOUNDG02` explain retention under sounding suppression
- `WRECKS02` explain retention
- `LNDARE` / `BOYSPP` `textAttributeKey` linkage
- point-symbol crop visibility in the traditional and simplified S-64 scenes

`FAIRWY` remains an informational scene-model difference in the Chart 1 harness; it is not treated as a runtime rendering defect in this follow-up.

## Verification

- `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target s52_instruction_string_parser_tests s52_lookup_model_tests s52_conditional_symbology_tests feature_symbolizer_tests point_symbol_tests s64_reference_smoke_tests chart1_s64_reference_harness_tests --parallel 1"`
- `@' ... CHART_VIEW_WRITE_REFERENCE=1 ... ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R '^runtime\\.chart1_s64_reference_harness$' --output-on-failure ... '@ | powershell -NoProfile -ExecutionPolicy Bypass -Command -`
- `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R '^(runtime\\.s52_instruction_string_parser|runtime\\.s52_lookup_model|runtime\\.s52_conditional_symbology|runtime\\.feature_symbolizer|runtime\\.point_symbol|runtime\\.s64_reference_smoke|runtime\\.chart1_s64_reference_harness)$' --output-on-failure"`
- `powershell -ExecutionPolicy Bypass -NoProfile -File C:/Users/zsh/source/repos/chart_view/scripts/opencpn_visual_delta_harness.ps1 -ManifestDir C:/Users/zsh/source/repos/chart_view/tests/data/opencpn -ObservationDir C:/Users/zsh/source/repos/chart_view/tests/data/reference -OutDir C:/Users/zsh/source/repos/chart_view/out/build/windows-msvc-debug/opencpn_visual_delta_manual`

Verification result:

- focused display-completeness matrix: `7/7` passed
- OpenCPN engineering delta summary:
  - `totalComparableFeatures = 9`
  - `totalFeatureDeltas = 0`
  - `totalCropDeltas = 0`
