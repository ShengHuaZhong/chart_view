# Phase 1 Demo Verification

Verification date: 2026-04-15

## Purpose

Close Phase 1 by confirming that the repository's single-chart path still follows the DLL-first architecture:

1. `chart_runtime.dll` owns chart-source reading, normalization, SENC v1 build/read, scene construction, and offscreen rendering.
2. `chart_qtwidgets.dll` hosts the runtime, binds viewport updates, and presents the runtime-owned RGBA frame in a Qt Widgets shell.
3. `chart_standalone.exe` remains the official demo host and does not absorb runtime parsing, SENC, scene, or render-core responsibilities.

## Verified Phase 1 display path

`source chart -> reader/normalizer -> unified dataset -> SENC v1 -> scene build -> runtime render -> Qt host display`

The path above was exercised through runtime smoke tests, the Qt Widgets bridge smoke, and the standalone host smokes listed below.

## Commands run

These commands were executed from the Visual Studio 2026 x64 Developer Command Prompt:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset build-windows-msvc-debug --target chart_runtime chart_qtwidgets chart_standalone runtime_api_tests s57_senc_smoke_tests cm93_senc_smoke_tests s101_senc_smoke_tests qtwidgets_smoke_tests
ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.api|runtime\.(s57_senc_smoke|cm93_senc_smoke|s101_senc_smoke)|qtwidgets\.smoke|chart_standalone\.(smoke|open_chart\.smoke|s57_host_smoke|cm93_host_smoke|s101_host_smoke)" --output-on-failure
```

## Verification matrix

| Layer | Test | Input | Result | Notes |
| --- | --- | --- | --- | --- |
| runtime | `runtime.api` | synthetic in-memory SENC pipeline | PASS | Confirms runtime load/render/frame-buffer API path. |
| runtime | `runtime.s57_senc_smoke` | canonical local S-57 chart `C1511781.000` | PASS | Confirms `source -> normalize -> SENC -> readback` for S-57. |
| runtime | `runtime.cm93_senc_smoke` | local CM93 root `C:/Users/zsh/Documents/chart_testdata/cm93` | PASS | Confirms Phase 1 CM93 decode/build/readback smoke. |
| runtime | `runtime.s101_senc_smoke` | synthetic scaffold S-101 bytes | PASS | Real S-101 parsing remains scaffold-only; smoke confirms the scaffold path still round-trips through SENC. |
| qtwidgets | `qtwidgets.smoke` | synthetic SENC fixture | PASS | Confirms runtime-backed widget presentation path remains wired through `chart_qtwidgets.dll`. |
| standalone | `chart_standalone.smoke` | no chart | PASS | Confirms demo host bootstrap and widget hosting. |
| standalone | `chart_standalone.open_chart.smoke` | checked-in synthetic S-101 fixture `tests/data/s101/smoke_dataset.101` | PASS | Confirms routed open-chart CLI path through the normal host flow. |
| standalone | `chart_standalone.s101_host_smoke` | checked-in synthetic S-101 fixture `tests/data/s101/smoke_dataset.101` | PASS | Requires non-zero visible geometry from the runtime-backed host path. |
| standalone | `chart_standalone.s57_host_smoke` | canonical local S-57 chart `C:/Users/zsh/Documents/chart_testdata/s57/C1511781.000` | PASS | Confirms full Phase 1 host display path on real S-57 data. |
| standalone | `chart_standalone.cm93_host_smoke` | local CM93 root `C:/Users/zsh/Documents/chart_testdata/cm93` | PASS | Confirms full Phase 1 host path on CM93 open/load/render invocation. |

## Summary

- S-57: full Phase 1 path passes from source chart to Qt host display.
- CM93: full Phase 1 host path passes; Phase 1 still validates decodable/open-load-render behavior rather than full CM93 feature extraction completeness.
- S-101: the repository now has a checked-in synthetic renderable smoke fixture for host-path verification, while real S-101 parsing remains scaffold-only until real data is available.
- The DLL-first layering remains intact: runtime owns chart/render core, qtwidgets is the bridge, and standalone is the host shell.
