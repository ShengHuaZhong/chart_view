# 15 — S57 -> SENC smoke

## Objective
Build and read back SENC from one S57 chart

## Phase
- Phase 1

## Layer
- verification

## Depends on
- 09
- 10
- 11
- 11a-chart-testdata-fixtures
- 12-s57-single-chart-normalizer

## In scope
- Wire S57Reader -> Normalizer -> SencWriter -> SencReader
- Log feature count and extent

## Out of scope
- No rendering yet

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md
- canonical local S57 fixture

## Required changes
- Wire S57Reader -> Normalizer -> SencWriter -> SencReader
- Log feature count and extent

## Deliverables
- s57 senc smoke test

## Done when
- Given one S57 chart, SENC build and readback succeed
- canonical local S57 chart completes:
  source -> normalize -> SENC -> read-back
- read-back metadata matches expected chart identity
- feature count is non-zero
- extent is valid

## Verification
- Run s57_senc_smoke
- load canonical S57
- normalize to `FeatureChartDataset`
- write SENC v1
- read SENC v1 back
- compare chart identity, feature count, and extent

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
