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

## Build rules
- Do not mix compiler toolchains in the same build tree.
- Do not hardcode developer-specific paths in source files.
- Prefer `CMakePresets.json` for local toolchain and Qt path selection.
