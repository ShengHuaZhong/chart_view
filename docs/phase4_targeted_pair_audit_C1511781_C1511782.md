# Phase 4 Targeted Pair Audit: `C1511781.000` / `C1511782.000`

Historical note:
- This document captures the blocker state observed before `64a-projected-quilt-unblock-for-known-real-pair`.
- The post-fix projected quilt behavior for the same real pair is documented in [phase4_projected_quilt_unblock_C1511781_C1511782.md](/C:/Users/zsh/source/repos/chart_view/docs/phase4_projected_quilt_unblock_C1511781_C1511782.md).

## Target Pair

- Data root: `C:/Users/zsh/Documents/chart_testdata/s57`
- Chart A: `C1511781.000`
- Chart B: `C1511782.000`
- Audit scope: explain why `chart_view` does not currently run this known real pair through the full
  `projected quilt -> S-52-backed symbolization -> Unicode-capable labels` path inside task 64.

## Discovery Path

This pair is discoverable in the current runtime smoke path.

- Raw S57 directory scan:
  - `findS57Charts()` in [s57_quilt_smoke_tests.cpp](/C:/Users/zsh/source/repos/chart_view/test/runtime/s57_quilt_smoke_tests.cpp:83) found both files under the configured root.
  - The targeted audit confirmed `A=1`, `B=1`, total `.000` files in the fixture root = `5`.
- Catalog path:
  - The targeted audit parsed both `.000` files with `S57Reader`, wrote temporary `.senc` files, then loaded them through [chart_catalog.cpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/catalog/chart_catalog.cpp:41).
  - `ChartCatalog::findById("C1511781")` and `ChartCatalog::findById("C1511782")` both succeeded.

Conclusion: task 64 is **not** blocked by pair discovery for this known pair.

## Observed Chart Metadata

The targeted audit produced these runtime-visible values after `S57Reader -> prepareDatasetForSmoke`:

| Chart | Usage band | Native scale | Extent | Feature count | Named features | Unicode-capable names | Text label candidates | S-52 baseline hits |
| --- | --- | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| `C1511781` | `3` | `372284` | `[117.55, 38.1597] -> [118.625, 38.7389]` | `534` | `0` | `0` | `0` | `0` |
| `C1511782` | `4` | `95169.1` | `[117.775, 38.2736] -> [118.053, 38.4217]` | `1430` | `0` | `0` | `0` | `0` |

## Geographic And Projected Relation

- Geographic relation:
  - `overlap`
- Projected relation in the Phase 4 Mercator display space:
  - `overlap`
- Projected extents:
  - `C1511781`: `[1.30856e+07, 4.57561e+06] -> [1.32053e+07, 4.6576e+06]`
  - `C1511782`: `[1.31107e+07, 4.59168e+06] -> [1.31416e+07, 4.61261e+06]`

Conclusion: task 64 is **not** blocked by “these two charts do not overlap”.

## Coverage, Quilt, Patch Result

The targeted audit used a viewport built from the union extent of the pair. The resulting Phase 4 base viewport scale was approximately `297827`.

### Coverage query

- `CoverageIndex::query()` returned both charts:
  - `C1511781`
  - `C1511782`

### Candidate ranking

The current selection policy in [chart_selection_policy.cpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/catalog/chart_selection_policy.cpp:21) sorts by:

1. `scaleDistance()`
2. `usagePenalty()`
3. source priority
4. stable ID/path key

At viewport scale `297827`:

- `C1511781` native scale `372284` is closer than `C1511782` native scale `95169.1`
- approximate log-scale distances:
  - `C1511781`: `0.2231`
  - `C1511782`: `1.1409`

So the current runtime ranks:

1. `C1511781`
2. `C1511782`

### Quilt inclusion and patch clipping

`QuiltPlanner::build()` in [quilt_planner.cpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/quilt/quilt_planner.cpp:83) then:

- projects the viewport and candidate chart extents
- takes the ranked list in order
- assigns the first accepted layer ownership of its projected visible region
- clips later layers against `ownedRegions` via `clipPatchesAgainstOwnedRegions()`

Observed result for this pair:

- `quilt ordered chart ids`: `C1511781`
- `quilt includes both target charts`: `false`
- only one runtime layer survived:
  - chart: `C1511781`
  - `patchCount=1`
  - `visibleArea=8.4869e+09`
  - `patchAreaSum=8.4869e+09`
  - `clipped=0`

`C1511782` never appears as a surviving quilt layer, which means the integrated dual-chart path is cut off before scene/render can honestly exercise “two-chart projected quilt”.

Conclusion: the primary task-64 runtime failure for this pair is **not** discovery or overlap. It is that the current quilt selection + patch ownership logic chooses the overview chart first and clips the detail chart out of the plan.

## S-52 And Label Findings

Even before fixing the quilt result, this pair has a second blocker:

- combined `S52LookupModel` baseline hits: `0`
- combined named features: `0`
- combined Unicode-capable names: `0`
- combined text label candidates: `0`
- visible projected labels after render/layout: `0`

This is consistent with the current `S57Reader` implementation:

- object class acronyms are synthesized as `"OBJ" + classCode` instead of real S-57 acronyms in [s57_reader.cpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/s57/s57_reader.cpp:350)
- `ATTF` attributes are stored under numeric placeholder keys like `"A116"` instead of semantic keys like `OBJNAM` in [s57_reader.cpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/s57/s57_reader.cpp:360)
- `NATF` is declared but not decoded into runtime feature attributes at all in [s57_reader.cpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/s57/s57_reader.cpp:21)

Because of that:

- [s52_lookup_model.hpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/portrayal/s52_lookup_model.hpp:32) cannot match real classes like `SOUNDG`, `DEPARE`, `COALNE`, etc.
- [label_layout.hpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/label_layout.hpp:54) cannot find `OBJNAM` / `NOBJNM`

Conclusion: even if the projected quilt policy started including both charts, the current real-parse output for this pair would still fail the integrated S-52 + Unicode label part of task 64.

## Final Root Cause Judgment

For `C1511781.000` / `C1511782.000`, task 64 is blocked by a **combination**:

1. The pair is real, discovered, and overlapping, but the runtime’s current quilt ranking/patch ownership keeps only `C1511781` in the quilt plan.
2. The current `S57Reader` semantic output is too shallow for this real pair: both charts produce zero S-52 baseline hits and zero label candidates.

This means the old blocker text “no overlapping / adjacent real pair exists” was incorrect for this targeted pair.

## Recommended Narrow Fix Slice

The narrowest honest follow-up is two-step and still runtime-owned:

1. `task 64 runtime quilt unblock`
   - Adjust the projected quilt selection/ownership behavior so this known overview/detail pair can survive into the same quilt plan instead of letting the broader chart fully own the viewport first.
   - Keep this change inside `chart_runtime` quilt policy code; do not move it into host/UI.

2. `task 64 semantic parse unblock`
   - Harden `S57Reader` so real S-57 object classes and at least the baseline text/name attributes used by Phase 4 (`OBJNAM`, `NOBJNM`, and the classes already covered by the repository’s baseline S-52 lookup model) survive real parse with semantic names.
   - This is a reader semantic hardening task, not a broad “expand S-52 lookup table everywhere” task.

Without both of those, the pair cannot honestly prove the full task 64 path.

## Verification Commands

Build:

```powershell
cmake --build --preset build-windows-msvc-debug --target s57_quilt_smoke_tests
```

Targeted audit run:

```powershell
& 'C:/Users/zsh/source/repos/chart_view/out/build/windows-msvc-debug/test/Debug/s57_quilt_smoke_tests.exe' '[targeted-pair]' -s --reporter console
```

Observed result:

- the targeted audit test itself passed and printed the metrics above
- the audit output concluded:
  - the pair is discovered
  - geographic and projected overlap both exist
  - coverage query returns both charts
  - the quilt plan keeps only `C1511781`
  - real parse still yields zero S-52 hits and zero label candidates
