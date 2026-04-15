# DLL-first Architecture

## Why DLL-first
The chart system is designed to be reused by other systems.
Therefore:
- `chart_runtime.dll` is the main product.
- `chart_qtwidgets.dll` is a convenience integration layer.
- `chart_standalone.exe` is only a reference host.

## Runtime API principles
- narrow public API
- opaque handles
- explicit lifecycle
- no QWidget / QRhi in public runtime ABI

## Integration modes
1. Runtime-only integration
   - load charts
   - build/load SENC
   - query data
   - render offscreen or through custom host

2. Qt Widgets integration
   - embed `ChartViewWidget`
   - connect host commands to runtime

3. Standalone demo host
   - full desktop shell for validation/demo
