# 16 — CM93 -> SENC smoke

## Objective
Build and read back SENC from one CM93 chart

## Phase
- Phase 1

## Layer
- verification

## Depends on
- 09
- 10
- 11
- 13

## In scope
- Wire CM93Reader -> Normalizer -> SencWriter -> SencReader

## Out of scope
- No rendering yet

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Wire CM93Reader -> Normalizer -> SencWriter -> SencReader

## Deliverables
- cm93 senc smoke test

## Done when
- Given one CM93 chart, SENC build and readback succeed

## Verification
- Run cm93_senc_smoke

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
