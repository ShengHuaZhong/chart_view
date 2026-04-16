# Blocked

- `64-s52-unicode-real-chart-smoke-s57`
  - Status: active blocker as of 2026-04-16.
  - Technical reason:
    The projected quilt blocker for the known OpenCPN-valid overlapping charts `C1511781.000` and `C1511782.000` has been resolved by `64a-projected-quilt-unblock-for-known-real-pair`. `chart_view` now finds both charts, computes overlapping geographic and projected extents, returns both through `CoverageIndex::query()`, ranks them as `C1511782` then `C1511781`, and keeps both in the final projected quilt plan with non-empty projected patches.

    Task 64 is still blocked, but only by the remaining real-parse semantic gap: the current `S57Reader` output for both charts still produces zero Phase 4 baseline `S52LookupModel` hits and zero `OBJNAM` / `NOBJNM`-driven label candidates, because real class acronyms and semantic attribute names are not yet preserved through parse.
  - Attempts made:
    1. Extended `test/runtime/s57_quilt_smoke_tests.cpp` with a fixed-pair audit path that targets only `C1511781.000` / `C1511782.000` instead of relying on generalized pair search.
    2. Adjusted `ChartSelectionPolicy` so coarser-than-viewport charts are treated as fallback coverage in quilt ownership order instead of owning overlap before finer charts.
    3. Added regression coverage in `chart_selection_policy_tests.cpp` and `quilt_planner_tests.cpp` for overview/detail projected quilt ownership.
    4. Rebuilt the targeted tests and reran the real-pair audit executable directly.
    5. Confirmed with printed audit metrics that both charts are now discovered, overlap, survive the quilt plan together, and still yield zero S-52 baseline hits / zero label candidates after real parse.
  - Verification evidence:
    - Build:
      `cmake --build --preset build-windows-msvc-debug --target chart_selection_policy_tests quilt_planner_tests s57_quilt_smoke_tests`
    - Targeted regression tests:
      `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(chart_selection_policy|quilt_planner)' --output-on-failure`
    - Targeted audit:
      `C:/Users/zsh/source/repos/chart_view/out/build/windows-msvc-debug/test/Debug/s57_quilt_smoke_tests.exe '[targeted-pair]' -s --reporter console`
    - Result:
      The targeted audit printed:
      - raw directory discovery: `A=1 B=1 total=5`
      - geographic relation: `overlap`
      - projected relation: `overlap`
      - coverage candidates: `C1511781, C1511782`
      - ranked candidates: `C1511782, C1511781`
      - quilt ordered chart ids: `C1511782, C1511781`
      - quilt includes both target charts: `1`
      - chart A patch area sum: `7.83845e+09`
      - chart B patch area sum: `6.48449e+08`
      - at least one quilt layer remained clipped after subtraction: `true`
      - combined counts: `s52Hits=0 named=0 unicodeNamed=0 textCandidates=0`
      - most likely root cause: `both charts reach quilt, but real parse leaves zero S-52 baseline lookup hits`
  - Why work cannot continue within task 64:
    Completing task 64 honestly now requires runtime changes beyond projected quilt ownership:
    1. real S-57 class acronyms must reach the Phase 4 S-52 lookup path, and
    2. real semantic text attributes such as `OBJNAM` / `NOBJNM` must reach multilingual label selection.
    Without those reader-semantic changes, the integrated real-chart path still cannot demonstrate the requested `projected quilt -> S-52-backed symbolization -> Unicode-capable labels` behavior.

- Historical note:
  - The previous `37-cm93-quilt-render-smoke` blocker was resolved by the CM93 runtime decode/extent hardening work in `src/runtime/cm93/`.
  - After the reader started preferring `geometry -> header -> cell-name fallback` extents and the OpenCPN-aligned cell-origin fallback was in place, `runtime.cm93_quilt_smoke` passed against the configured CM93 dataset root `C:/Users/zsh/Documents/chart_testdata/cm93`.
