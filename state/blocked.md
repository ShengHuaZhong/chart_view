# Blocked

- None currently.
- Historical note:
  - The previous `37-cm93-quilt-render-smoke` blocker was resolved by the CM93 runtime decode/extent hardening work in `src/runtime/cm93/`.
  - After the reader started preferring `geometry -> header -> cell-name fallback` extents and the OpenCPN-aligned cell-origin fallback was in place, `runtime.cm93_quilt_smoke` passed against the configured CM93 dataset root `C:/Users/zsh/Documents/chart_testdata/cm93`.
