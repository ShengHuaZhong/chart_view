# Phase 2 Demo Verification

Verification date: 2026-04-16

## Purpose

Close Phase 2 by confirming that the repository's multi-chart path still follows the DLL-first architecture:

1. `chart_runtime.dll` owns catalog loading, coverage lookup, quilt planning, zoom evaluation, multi-chart scene construction, and offscreen rendering.
2. `chart_qtwidgets.dll` remains a Qt Widgets bridge and does not absorb catalog, quilt, or zoom policy logic.
3. `chart_standalone.exe` remains the official host shell and only routes a chart directory into the runtime-owned multi-chart flow.

## Verified Phase 2 display path

`chart directory or SENC set -> ChartCatalog -> CoverageIndex -> QuiltPlanner -> ZoomPolicy -> multi-chart SceneSnapshot -> runtime render -> Qt host display`

The path above was exercised through the runtime quilt smokes listed below and through a standalone directory smoke that opens a multi-chart directory through the host shell.

## Commands run

These commands were executed from the Visual Studio 2026 x64 Developer Command Prompt:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset build-windows-msvc-debug --target chart_runtime runtime_api_tests chart_standalone s57_quilt_smoke_tests cm93_quilt_smoke_tests s101_quilt_smoke_tests
ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.(s57_quilt_smoke|cm93_quilt_smoke|s101_quilt_smoke)|chart_standalone\.open_chart_directory\.smoke" --output-on-failure
```

## Verification matrix

| Layer | Test | Input | Result | Notes |
| --- | --- | --- | --- | --- |
| runtime | `runtime.s57_quilt_smoke` | real local S-57 charts from `C:/Users/zsh/Documents/chart_testdata/s57` | PASS | Confirms `reader -> SENC -> catalog -> coverage -> quilt -> zoom -> scene -> RHI render` on multi-chart S-57 data. |
| runtime | `runtime.cm93_quilt_smoke` | real local CM93 root `C:/Users/zsh/Documents/chart_testdata/cm93` | PASS | Confirms the hardened CM93 runtime path can participate in multi-chart quilt planning and zoom verification. |
| runtime | `runtime.s101_quilt_smoke` | checked-in synthetic two-chart S-101 sources | PASS | Confirms deterministic multi-chart S-101 quilt/zoom coverage without depending on external S-101 data. |
| standalone | `chart_standalone.open_chart_directory.smoke` | checked-in synthetic S-101 directory `tests/data/s101/quilt_directory` | PASS | Confirms the official host can route a chart directory into runtime-owned multi-chart mode without moving catalog/quilt logic into the host shell. |

## Summary

- S-57: Phase 2 runtime multi-chart quilt and zoom path passes.
- CM93: Phase 2 runtime multi-chart quilt and zoom path passes on configured local data.
- S-101: Phase 2 runtime quilt path passes on checked-in deterministic fixtures, and the official host directory route passes on a checked-in multi-chart directory fixture.
- The DLL-first layering remains intact:
  - `chart_runtime.dll` owns chart selection, quilt planning, zoom policy, scene build, and rendering.
  - `chart_qtwidgets.dll` remains the host bridge only.
  - `chart_standalone.exe` remains the demo host shell only.
