# Task 64 Closeout: S-52 + Unicode Real-Chart Smoke (S57)

## Task

- `64-s52-unicode-real-chart-smoke-s57`

## Scope

This closeout records the formal completion of the Phase 4 real-chart integrated smoke for S57.

The closeout is based on the fixed real pair:

- `C1511781.000`
- `C1511782.000`

under the configured fixture root:

- `C:/Users/zsh/Documents/chart_testdata/s57`

This task closes out the smoke baseline only. It does not claim S-64 compliance or full ECDIS completeness.

## What Was Already Unblocked Before Closeout

Task 64 depended on two narrow runtime fixes already completed in the local worktree:

1. [phase4_projected_quilt_unblock_C1511781_C1511782.md](/C:/Users/zsh/source/repos/chart_view/docs/phase4_projected_quilt_unblock_C1511781_C1511782.md)
   - resolved the projected quilt inclusion blocker for the known overview/detail pair
2. [phase4_s57_semantic_unblock_C1511781_C1511782.md](/C:/Users/zsh/source/repos/chart_view/docs/phase4_s57_semantic_unblock_C1511781_C1511782.md)
   - resolved the real S57 semantic-retention blocker so the pair could hit the existing S-52 and multilingual label paths

This closeout reruns the integrated path and confirms that task 64 now passes end-to-end.

## Formal Verification Evidence

### Build

```powershell
cmake --build --preset build-windows-msvc-debug --target s57_reader_tests s57_quilt_smoke_tests
```

Observed result:

- `ninja: no work to do.`

### CTest

```powershell
ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.(s57_reader|s57_quilt_smoke)" --output-on-failure
```

Observed result:

- `runtime.s57_reader` passed
- `runtime.s57_quilt_smoke` passed

### Fixed-pair targeted integrated smoke

```powershell
& 'C:/Users/zsh/source/repos/chart_view/out/build/windows-msvc-debug/test/Debug/s57_quilt_smoke_tests.exe' '[targeted-pair]' -s --reporter console
```

Observed result:

- targeted pair audit passed
- all 43 assertions passed
- the run was not skipped

## Fixed-pair Closeout Findings

### Pair discovery and quilt inclusion

- raw directory discovery:
  - `A=1`
  - `B=1`
  - total `.000` files in the fixture root: `5`
- catalog inclusion:
  - `findById(A)=1`
  - `findById(B)=1`
- coverage candidates:
  - `C1511781`
  - `C1511782`
- ranked candidates:
  - `C1511782`
  - `C1511781`
- quilt ordered chart ids:
  - `C1511782`
  - `C1511781`
- quilt includes both target charts:
  - `1`

### Projected quilt / patch evidence

- geographic relation:
  - `overlap`
- projected relation:
  - `overlap`
- patch evidence:
  - `C1511782 patchAreaSum = 6.48449e+08`
  - `C1511781 patchAreaSum = 7.83845e+09`
  - overview layer `clipped = 1`

This confirms the smoke is using a real two-chart projected quilt path, not a single-chart fallback and not a bypassed clipping path.

### S-52 / label evidence

- `C1511781`
  - `named = 86`
  - `unicodeNamed = 48`
  - `textCandidates = 86`
  - `s52Hits = 183`
- `C1511782`
  - `named = 134`
  - `unicodeNamed = 21`
  - `textCandidates = 134`
  - `s52Hits = 903`
- combined:
  - `s52Hits = 1086`
  - `named = 220`
  - `unicodeNamed = 69`
  - `textCandidates = 220`
  - `rawNobjnm = 216`
- visible projected labels:
  - `total = 68`
  - `unicode = 34`

This confirms the smoke is now honestly exercising:

- projected quilt composition
- S-52-backed symbolization
- Unicode-capable label flow

## Closeout Judgment

`64-s52-unicode-real-chart-smoke-s57` is complete.

The fixed real pair `C1511781.000` / `C1511782.000` now proves that one real S57 path runs through:

- projected quilt planning
- runtime-owned patch clipping
- S-52-backed symbolization
- Unicode-capable label extraction and visible label rendering

within the current Phase 4 baseline.

This is a smoke-level completion only:

- it is not an S-64 compliance statement
- it is not a claim of full S-52 catalogue coverage
- it does not close out task 65 or later Phase 4 verification work
