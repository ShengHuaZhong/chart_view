# chart_view

`chart_view` is a DLL-first marine chart system skeleton.

The repository now treats these targets as the product hierarchy:

- `chart_runtime.dll`: narrow runtime C API with opaque handles and build metadata.
- `chart_qtwidgets.dll`: Qt Widgets host layer that embeds the runtime without absorbing core chart logic.
- `chart_standalone.exe`: official demo host for smoke testing and manual validation.

This iteration establishes the build boundaries, install/export flow, and Windows-first Qt 6 scaffolding. It does not yet implement chart parsing, SENC generation, quilt/zoom policy, or full nautical symbology.

## Quick Start

Open a Visual Studio Developer Command Prompt or Developer PowerShell, then point CMake at a Qt 6 desktop installation through `CMAKE_PREFIX_PATH`.

```powershell
cmake --preset windows-msvc-debug -DCMAKE_PREFIX_PATH=C:\Qt\6.11.0\msvc2022_64
cmake --build --preset build-windows-msvc-debug
ctest --preset test-windows-msvc-debug
```

`chart_standalone` supports:

- `--version`
- `--smoke-test`

## Repository Notes

- Main UI shell: Qt Widgets
- Main rendering direction: Qt 6 RHI
- Public runtime ABI: C API only
- Current official build focus: Windows + MSVC + Qt 6

## More Details

- [Building Details](README_building.md)
- [Dependency Setup](README_dependencies.md)
- [Troubleshooting](README_troubleshooting.md)
- [Docker / Devcontainer Notes](README_docker.md)
