## Dependencies

The active dependency surface is intentionally narrow:

- Visual Studio / MSVC
- CMake 3.29+
- Ninja
- Qt 6 desktop build with `Core`, `Gui`, and `Widgets`
- Catch2 for tests

## Windows Setup

### Visual Studio

Install Visual Studio with the Desktop C++ workload.

### CMake and Ninja

Install both tools and ensure they are available on `PATH`.

### Qt 6

Install a Qt 6 desktop build that matches the compiler toolchain you plan to use. For the current CI and Windows-first workflow, that means an MSVC desktop package.

Set `CMAKE_PREFIX_PATH` to the Qt prefix when configuring, for example:

```powershell
cmake --preset windows-msvc-debug -DCMAKE_PREFIX_PATH=C:\Qt\6.11.0\msvc2022_64
```

## Notes

- Linux and macOS compatibility are future follow-up tasks, not part of the current supported mainline.
- Fuzzing, WebAssembly deployment, and terminal sample dependencies were removed from the primary build graph during the DLL-first migration.
