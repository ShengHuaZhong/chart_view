# 17 — S-101 -> SENC smoke

## Objective
Build and read back SENC from one S-101 chart

## Phase
- Phase 1

## Layer
- verification

## Depends on
- 09
- 10
- 11
- 14

## In scope
- Wire S101Reader -> Normalizer -> SencWriter -> SencReader

## Out of scope
- No rendering yet

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Wire S101Reader -> Normalizer -> SencWriter -> SencReader

## Deliverables
- s101 senc smoke test

## Done when
- Given one S-101 chart, SENC build and readback succeed

## Verification
- Run s101_senc_smoke

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
