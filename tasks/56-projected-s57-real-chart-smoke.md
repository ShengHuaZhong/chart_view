# 56 — Projected S57 real-chart smoke

## Objective
Real-chart S57 smoke for projected multi-chart rendering

## Phase
- Phase 4

## Layer
- integration / smoke

## Depends on
- 55

## In scope
- Add a projected S57 multi-chart smoke path using real chart data when available
- Exercise reference-chart selection and projected quilt composition
- Prove the runtime can render a projected S57 quilt without regressing existing host boundaries

## Out of scope
- No S-52 portrayal yet
- No Unicode label requirements yet
- No full ECDIS compliance claims

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add or extend a real-chart S57 quilt smoke target
- Assert non-empty projected rendering output
- Document any test-data gating used by the smoke path

## Deliverables
- Projected S57 real-chart smoke target
- Updated verification notes if needed

## Done when
A real S57 multi-chart path proves that projected quilt rendering works before Phase 4 portrayal and text upgrades begin.

## Verification
- Projected S57 quilt smoke test
- Relevant targeted runtime / host smoke if already used by the repository

## Notes
- This task closes the projection-only substage before S-52 work begins.
- Do not silently pull S-52 logic into this task.
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
