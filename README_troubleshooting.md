## Troubleshooting

### CMake cannot find Qt6

Pass the Qt installation prefix explicitly:

```powershell
cmake --preset windows-msvc-debug -DCMAKE_PREFIX_PATH=C:\Qt\6.11.0\msvc2022_64
```

### `chart_standalone --smoke-test` fails with a Qt platform plugin error

This usually means the Qt plugin directory is not available to the process. The test suite sets `QT_PLUGIN_PATH` automatically, but manual local runs may still need a Qt-enabled environment or an installed/deployed app layout.

### A preset fails because Ninja is missing

Install Ninja or configure manually with a Visual Studio generator instead of the provided preset.

### A preset cannot find `cl.exe`

Open a Visual Studio Developer Command Prompt or Developer PowerShell before running the `windows-msvc-*` presets. Those presets expect the MSVC compiler environment to be available.

### A stale build directory complains about generator mismatch

Remove the old build directory for that preset or choose a new binary directory before reconfiguring.
