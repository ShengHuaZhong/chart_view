# Phase 4 Projected Quilt Unblock: `C1511781.000` / `C1511782.000`

## Scope

- Subtask: `64a-projected-quilt-unblock-for-known-real-pair`
- Data root: `C:/Users/zsh/Documents/chart_testdata/s57`
- Fixed pair:
  - `C1511781.000`
  - `C1511782.000`
- Goal:
  - keep both charts alive in the same runtime-owned projected quilt plan
  - preserve projected patch clipping and seam handling
  - avoid hardcoded chart-id exceptions or disabling patch subtraction

This subtask only addresses projected quilt inclusion. It does not expand S-52 lookup coverage or harden real S-57 semantic parsing.

## Why Only `C1511781` Survived Before

The pre-fix audit for this pair is recorded in [phase4_targeted_pair_audit_C1511781_C1511782.md](/C:/Users/zsh/source/repos/chart_view/docs/phase4_targeted_pair_audit_C1511781_C1511782.md).

At the pair's union viewport scale of about `297827`, the old selection policy in
[chart_selection_policy.cpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/catalog/chart_selection_policy.cpp)
sorted overlapping candidates primarily by log-scale distance to the viewport:

- `C1511781` native scale `372284` was closer to the viewport than `C1511782` native scale `95169.1`
- so runtime ranked:
  - `C1511781`
  - `C1511782`

`QuiltPlanner::build()` then treated the first accepted layer as owning its full projected visible region and clipped later candidates against that owned region through
[quilt_planner.cpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/quilt/quilt_planner.cpp).

For this overview/detail pair, that ordering was backwards for quilt ownership:

- the broader chart `C1511781` claimed the overlap first
- `C1511782` was fully subtracted away
- the resulting quilt plan contained only `C1511781`

The problem was not chart discovery, geographic overlap, projected overlap, coverage query, or PROJ itself. It was the ownership order used before patch subtraction.

## Narrow Fix

The fix stays inside runtime-owned chart selection:

- [chart_selection_policy.hpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/catalog/chart_selection_policy.hpp)
- [chart_selection_policy.cpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/catalog/chart_selection_policy.cpp)

The selection policy now applies a `coarsenessPenalty()` before usage and scale-distance tiebreakers:

- charts coarser than the display scale are treated as fallback coverage
- same-scale or finer charts rank ahead of coarser overlaps

For the real pair:

- `C1511782` is finer than the `297827` viewport scale
- `C1511781` is coarser than the `297827` viewport scale
- the runtime now ranks:
  - `C1511782`
  - `C1511781`

This is a narrow, general rule:

- it does not hardcode `C1511781` or `C1511782`
- it does not disable `clipPatchesAgainstOwnedRegions()`
- it still allows the overview chart to contribute fallback coverage outside the detailed chart's footprint

## Post-fix Quilt Inclusion And Patch Behavior

The targeted real-pair audit now prints:

- `coverage candidates: C1511781, C1511782`
- `ranked candidates: C1511782, C1511781`
- `quilt ordered chart ids: C1511782, C1511781`
- `quilt includes both target charts: 1`

Observed layer results:

| Layer | Draw order | Patch count | Patch area sum | Clipped |
| --- | ---: | ---: | ---: | --- |
| `C1511782` | `0` | `1` | `6.48449e+08` | `false` |
| `C1511781` | `1` | `4` | `7.83845e+09` | `true` |

Important consequences:

- both charts survive into the same quilt plan
- both charts keep non-empty projected patches
- the broader chart still gets clipped, so patch subtraction is still active
- the result matches the intended quilt semantics for an overview/detail pair:
  - detail owns the overlap
  - overview fills the remainder

This means the unblock is real. The test is not passing because clipping was bypassed.

## Tests Added Or Strengthened

- [chart_selection_policy_tests.cpp](/C:/Users/zsh/source/repos/chart_view/test/runtime/chart_selection_policy_tests.cpp)
  - added a regression proving a finer overlap ranks ahead of a coarser overview at the pair's viewport scale
- [quilt_planner_tests.cpp](/C:/Users/zsh/source/repos/chart_view/test/runtime/quilt_planner_tests.cpp)
  - added a regression proving both layers survive, both keep non-empty projected patches, and the overview remains clipped
- [s57_quilt_smoke_tests.cpp](/C:/Users/zsh/source/repos/chart_view/test/runtime/s57_quilt_smoke_tests.cpp)
  - strengthened the targeted real-pair audit to require:
    - both target charts in the quilt plan
    - non-empty projected patches for both
    - evidence that at least one layer still has clipped visible area

## Why This Is The Narrowest Acceptable Fix

This subtask intentionally avoids broader changes:

- no host or widget changes
- no runtime public ABI changes
- no S-52 lookup-table expansion
- no S-57 semantic parse work
- no seam or clipping bypass
- no general quilt rewrite

The change fixes the single runtime layer that was causing the known real pair to collapse from a two-chart quilt into a one-chart quilt.

## Remaining Task 64 Blocker

Task 64 is still blocked, but the blocker is now narrower:

- both charts now reach the quilt plan and the rendered scene
- the remaining problem is semantic:
  - real parse still produces `s52Hits=0`
  - real parse still produces `named=0`
  - real parse still produces `unicodeNamed=0`
  - real parse still produces `textCandidates=0`

So the next honest runtime task is not more quilt work. It is S-57 semantic hardening so the real pair can hit baseline S-52 lookup and Unicode-capable label extraction.

## Verification

Build:

```powershell
cmake --build --preset build-windows-msvc-debug --target chart_selection_policy_tests quilt_planner_tests s57_quilt_smoke_tests
```

Targeted regression tests:

```powershell
ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.(chart_selection_policy|quilt_planner)" --output-on-failure
```

Targeted real-pair audit:

```powershell
& 'C:/Users/zsh/source/repos/chart_view/out/build/windows-msvc-debug/test/Debug/s57_quilt_smoke_tests.exe' '[targeted-pair]' -s --reporter console
```

Observed result:

- `runtime.chart_selection_policy` passed
- `runtime.quilt_planner` passed
- targeted real-pair audit passed
- the real-pair audit confirmed both charts now survive into the quilt plan and that patch clipping remains active
