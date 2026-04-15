## Build Instructions

This repository is currently optimized for Windows + MSVC + Qt 6.

### Prerequisites

- Visual Studio with the Desktop C++ workload
- CMake 3.29+
- Ninja
- Qt 6 desktop build for MSVC

Run the commands below from a Visual Studio Developer Command Prompt or Developer PowerShell so that `cl.exe`, the MSVC linker, and the VS-bundled Ninja toolchain are available.

`CMAKE_PREFIX_PATH` must point at the Qt installation prefix, for example `C:\Qt\6.11.0\msvc2022_64`.

### Configure

Using presets:

```powershell
cmake --preset windows-msvc-debug -DCMAKE_PREFIX_PATH=C:\Qt\6.11.0\msvc2022_64
```

Without presets:

```powershell
cmake -S . -B .\build -G "Ninja Multi-Config" -DCMAKE_PREFIX_PATH=C:\Qt\6.11.0\msvc2022_64
```

### Build

```powershell
cmake --build --preset build-windows-msvc-debug
```

Or:

```powershell
cmake --build .\build --config Debug
```

### Test

```powershell
ctest --preset test-windows-msvc-debug
```

Or:

```powershell
ctest --test-dir .\build -C Debug --output-on-failure
```

### Install

```powershell
cmake --install .\build --config Debug --prefix .\out\install\manual-debug
```

The install tree exports `chart_view::chart_runtime` and `chart_view::chart_qtwidgets`, and deploys the standalone host on Windows.
