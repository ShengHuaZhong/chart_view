# Blocked

- None active as of 2026-04-16.

## Historical notes

- `64-s52-unicode-real-chart-smoke-s57`
  - The projected quilt blocker was resolved by `64a-projected-quilt-unblock-for-known-real-pair`.
  - The remaining real-parse semantic blocker was resolved by `64b-s57-semantic-baseline-unblock-for-real-smoke`.
  - The fixed real pair `C1511781.000` / `C1511782.000` now reaches the integrated projected quilt + S-52 + label path with combined `s52Hits=1086`, `named=220`, `unicodeNamed=69`, and `textCandidates=220`.
  - Verification evidence:
    - `cmake --build --preset build-windows-msvc-debug --target s57_reader_tests s57_quilt_smoke_tests`
    - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.(s57_reader|s57_quilt_smoke)" --output-on-failure`
    - `C:/Users/zsh/source/repos/chart_view/out/build/windows-msvc-debug/test/Debug/s57_quilt_smoke_tests.exe '[targeted-pair]' -s --reporter console`
  - Result:
    - `runtime.s57_reader` passed
    - `runtime.s57_quilt_smoke` passed
    - the targeted pair audit passed and printed non-zero semantic and label counters for the known real pair

- Historical note:
  - The previous `37-cm93-quilt-render-smoke` blocker was resolved by the CM93 runtime decode/extent hardening work in `src/runtime/cm93/`.
  - After the reader started preferring `geometry -> header -> cell-name fallback` extents and the OpenCPN-aligned cell-origin fallback was in place, `runtime.cm93_quilt_smoke` passed against the configured CM93 dataset root `C:/Users/zsh/Documents/chart_testdata/cm93`.
