# Build Environment

## Toolchain
- CMake >= 3.28
- C++ standard: C++20
- Recommended compiler (Windows): MSVC 2022 x64
- Qt: 6.8.x (Widgets + RHI-capable build)
- Package manager: vcpkg (manifest mode preferred)

## Qt path policy
- If Qt is installed from official packages, set `CMAKE_PREFIX_PATH` in `CMakePresets.json`.
- If Qt comes from vcpkg, use `CMAKE_TOOLCHAIN_FILE` and do not hardcode Qt paths in `CMakeLists.txt`.

## vcpkg
Recommended triplet:
- `x64-windows`

Recommended dependencies for early phases:
- qtbase
- gdal
- fmt
- spdlog
- catch2 (or doctest / gtest if preferred)

Additional dependency for Phase 4 projected-display work:
- proj

## Phase 4 projection dependency note
- Keep PROJ ownership inside `chart_runtime`.
- Do not expose PROJ handles through the runtime public ABI.
- Do not move display-projection ownership into `chart_qtwidgets` or `chart_standalone`.

## Phase 4 projected S57 smoke gating
- `runtime.s57_quilt_smoke` uses real S57 data only when `CHARTSYS_ENABLE_REAL_CHART_TESTS=ON` and `CHARTSYS_S57_TESTDATA_ROOT` points to a valid S57 dataset root.
- The projected quilt smoke will `SKIP` when fewer than two readable `.000` charts with valid extents are available, or when no overlapping / adjacent pair can be formed for quilt composition.
- Host verification stays separate through `chart_standalone.s57_host_smoke`; projected-display work must not require host-owned PROJ state.

## Build rules
- Do not mix compiler toolchains in the same build tree.
- Do not hardcode developer-specific paths in source files.
- Prefer `CMakePresets.json` for local toolchain and Qt path selection.
