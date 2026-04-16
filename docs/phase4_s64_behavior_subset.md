# Phase 4 Selected S-64-Inspired Behavior Subset

## Scope

- Task: `65-s64-reference-behavior-smoke`
- Layer: `verification / portrayal behavior`
- Purpose:
  - define the narrow S-64-inspired behavior subset that this repository verifies automatically
  - keep the subset honest, explicit, and suitable for smoke-level regression protection

This document does **not** claim full S-64 pass or ECDIS compliance.

## Normative Boundary

- IHO S-52 / Annex A / S-64 are the normative behavior sources for this task.
- OpenCPN may still be used for engineering cross-checks, but it is not the normative truth source.
- The repository only verifies a small runtime-owned subset that matches the current Phase 4 baseline.

## Selected Behavior Subset

The current repository verifies the following explicit subset:

1. `S52 display settings suppress soundings`
   - when `showSoundings = false`, sounding features do not contribute visible symbol output
   - this is treated as a runtime-owned portrayal decision, not a host decision

2. `S52 point-symbol mode switches buoy / beacon variants`
   - when `pointSymbolMode = kSimplified`, buoy / beacon point-symbol instructions switch from the traditional asset variant to the simplified asset variant
   - this is validated through the emitted S-52 instruction path and visible renderer output

3. `S52 text-label visibility is settings-controlled`
   - when `showTextLabels = false`, the runtime removes label instructions and suppresses visible label output
   - when labels are enabled, visible label output returns

4. `Unicode-capable national names survive the runtime label path`
   - when a feature carries a valid national-language name in `NOBJNM`, the runtime prefers it over `OBJNAM`
   - the rendered label path must remain Unicode-capable instead of dropping to ASCII-only behavior

5. `Projected multi-chart integration remains covered by task 64`
   - the fixed real pair `C1511781.000` / `C1511782.000` remains the repository's Phase 4 proof that projected quilt + S-52 + Unicode labels work together on real S57 data
   - task 65 does not replace that integrated smoke; it adds a narrower reference-behavior subset on top of it

## Test Coverage

The selected subset is covered by:

- [s64_reference_smoke_tests.cpp](/C:/Users/zsh/source/repos/chart_view/test/runtime/s64_reference_smoke_tests.cpp)
  - synthetic runtime smoke for:
    - sounding suppression
    - simplified buoy variant selection
    - label enable / disable behavior
    - Unicode-capable `NOBJNM` label selection

- [s57_quilt_smoke_tests.cpp](/C:/Users/zsh/source/repos/chart_view/test/runtime/s57_quilt_smoke_tests.cpp)
  - fixed-pair real-chart integrated evidence for:
    - projected quilt inclusion
    - non-zero S-52 baseline hits
    - non-zero named / Unicode-capable labels
    - visible projected labels on real data

## Explicitly Not Covered

Still out of scope for this task:

- full S-64 catalogue behavior coverage
- any compliance or type-approval statement
- full day / dusk / night palette behavior
- broader mariner-parameter matrices
- full S-101 Phase 4 portrayal
- host-owned validation logic

## Environment Notes

- Some local direct runs may print `proj.db` warnings when PROJ is initialized from the desktop environment.
- In this repository, that is treated as an **environment warning**, not automatically as a runtime implementation failure, as long as:
  - `ProjectionContext` is still valid, and
  - the relevant smoke tests pass
- If projection initialization becomes invalid or the reference smoke fails because of PROJ resource lookup, that should be treated as a real blocker for Phase 4 verification.
