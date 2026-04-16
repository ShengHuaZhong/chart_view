# Phase 3 Demo Verification

Verification date: 2026-04-16

## Purpose

Close Phase 3 by confirming that the repository has progressed from geometry-visible rendering to a semantically readable nautical-chart baseline while preserving the DLL-first architecture:

1. `chart_runtime.dll` owns portrayal rule resolution, display priority, symbol rasterization, text labels, and final offscreen rendering.
2. `chart_qtwidgets.dll` remains the Qt Widgets bridge and does not absorb runtime portrayal or rendering policy.
3. `chart_standalone.exe` remains the official host shell and does not absorb runtime-owned symbolization logic.

## Verified Phase 3 display path

`chart data or SENC -> FeatureSymbolizer -> DisplayPriorityModel -> point / line / area / text renderers -> runtime frame render`

The path above was exercised through the runtime verification matrix listed below, including dedicated S57 and S-101 symbolized smoke tests that round-trip chart samples through SENC before rendering a frame.

## Commands run

These commands were executed from the Visual Studio 2026 x64 Developer Command Prompt:

```powershell
cmake --build --preset build-windows-msvc-debug --target portrayal_registry_tests feature_symbolizer_tests display_priority_tests point_symbol_tests line_symbol_tests area_symbol_tests label_tests s57_rule_table_tests s101_rule_table_tests feature_renderer_tests s57_symbolized_smoke_tests s101_symbolized_smoke_tests
ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.(portrayal_registry|feature_symbolizer|display_priority|point_symbol|line_symbol|area_symbol|label|s57_rule_table|s101_rule_table|feature_renderer|s57_symbolized_smoke|s101_symbolized_smoke)" --output-on-failure
```

## Verification matrix

| Layer | Test | Input | Result | Notes |
| --- | --- | --- | --- | --- |
| runtime | `runtime.portrayal_registry` | built-in portrayal registry defaults | PASS | Confirms the runtime-owned registry still resolves rule defaults without host involvement. |
| runtime | `runtime.feature_symbolizer` | synthetic S57 / S-101 features plus SENC readback fixtures | PASS | Confirms semantic style-key resolution, including S57 and S-101 rule-table coverage. |
| runtime | `runtime.display_priority` | synthetic overlapping area / line / point features | PASS | Confirms stable layer grouping and ordering for semantic styles. |
| runtime | `runtime.point_symbol` | built-in runtime glyph atlas | PASS | Confirms point-symbol rendering for semantic point styles. |
| runtime | `runtime.line_symbol` | built-in runtime line-pattern styles | PASS | Confirms channel / contour line styling through the runtime renderer path. |
| runtime | `runtime.area_symbol` | built-in runtime area pattern/fill path | PASS | Confirms area fill and pattern rendering remain available for semantic area styles. |
| runtime | `runtime.label` | runtime-owned bitmap text label path | PASS | Confirms extraction, layout, and rendering of basic chart labels. |
| runtime | `runtime.s57_rule_table` | selected S57 class acronyms | PASS | Confirms key S57 objects resolve to semantic portrayal styles instead of geometry-only fallback. |
| runtime | `runtime.s101_rule_table` | selected S-101 semantic class names and scaffold aliases | PASS | Confirms key S-101 objects resolve to semantic portrayal styles through an explicit S-101 rule table. |
| runtime | `runtime.feature_renderer` | synthetic rendered scenes with portrayal overrides | PASS | Confirms the integrated runtime renderer combines semantic styles, display priority, and labels in a frame. |
| runtime | `runtime.s57_symbolized_smoke` | synthetic S57 sample round-tripped through SENC | PASS | Confirms semantic S57 area / line / point styling, point-over-line priority, and basic labels in one end-to-end frame. |
| runtime | `runtime.s101_symbolized_smoke` | synthetic S-101 sample round-tripped through SENC | PASS | Confirms semantic S-101 area / line / point styling, point-over-line priority, and basic labels in one end-to-end frame. |

## Summary

- S57: semantic point / line / area styles, display priority, and basic labels pass through a dedicated end-to-end symbolized smoke.
- S-101: the same semantic rendering baseline now passes through an explicit S-101 rule-table path and a dedicated end-to-end symbolized smoke.
- The runtime has advanced from geometry-only visibility to a semantically readable baseline with:
  - portrayal rule lookup
  - display priority layering
  - point / line / area symbolization
  - basic text labels
- The DLL-first layering remains intact:
  - `chart_runtime.dll` owns the portrayal and rendering baseline.
  - `chart_qtwidgets.dll` remains the host bridge only.
  - `chart_standalone.exe` remains the demo shell only.

## Completion note

Phase 3 is complete at the repository roadmap level as a narrow baseline for semantic nautical-chart rendering. This is not a full production portrayal catalogue, but it does satisfy the roadmap transition from geometry-visible output to semantically readable chart output.
