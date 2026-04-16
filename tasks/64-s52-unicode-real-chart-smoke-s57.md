# 64 — S-52 + Unicode real-chart smoke (S57)

## Objective
Integrated real-chart S57 smoke for projected quilting + S-52 + Unicode labels

## Phase
- Phase 4

## Layer
- integration / smoke

## Depends on
- 63

## In scope
- Exercise the integrated Phase 4 S57 path on real charts when test data is available
- Cover projected quilt rendering, S-52-backed symbolization, and Unicode-capable labels in one smoke path
- Keep verification centered on the runtime-owned render path

## Out of scope
- No S-64 reference-behavior matrix yet
- No full ECDIS compliance claim
- No S-101 expansion

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add an integrated real-chart smoke target or extend an existing one
- Assert non-empty projected rendering and visible symbolized output
- Add label-related assertions when the fixture data allows it

## Deliverables
- Integrated S57 Phase 4 smoke path

## Done when
One real-chart S57 path proves that projected quilting, S-52 portrayal, and Unicode-capable labels work together through the runtime renderer.

## Verification
- Integrated Phase 4 S57 smoke test
- Relevant targeted runtime / host smoke commands

## Notes
- Use real-data gating when necessary, but keep the baseline honest.
- This task is still a smoke, not a compliance certificate.
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
