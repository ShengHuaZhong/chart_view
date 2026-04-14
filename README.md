# ChartSystem

DLL-first marine chart system build skeleton.

## Products

- `chart_runtime` (shared library): runtime/chart pipeline/render-core center.
- `chart_qtwidgets` (shared library): Qt Widgets host bridge for runtime.
- `chart_standalone` (executable): official thin Qt Widgets reference host.

## Build

```bash
cmake --preset unixlike-gcc-debug
cmake --build out/build/unixlike-gcc-debug
ctest --test-dir out/build/unixlike-gcc-debug --output-on-failure
```

## Package consumption

Downstream projects can use:

```cmake
find_package(ChartSystem REQUIRED)
target_link_libraries(app PRIVATE ChartSystem::runtime ChartSystem::qtwidgets)
```
