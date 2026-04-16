# 54 — Projected scene and coverage space

## Objective
Move scene and coverage math into projected display space

## Phase
- Phase 4

## Layer
- scene / coverage / quilt-prep

## Depends on
- 53

## In scope
- Convert scene culling inputs from geographic extent math to projected display-space math
- Convert coverage-index overlap calculations to projected display space
- Introduce projected extents / projected bounds helpers reused by scene and coverage code
- Preserve current DLL-first boundaries

## Out of scope
- No seam-aware quilt patch clipping yet
- No S-52 lookup work yet
- No text-layout rewrite yet

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Update scene-building and coverage helpers to consume projected inputs
- Add focused tests for projected overlap / projected culling
- Keep projection ownership in runtime internals only

## Deliverables
- Projected scene / coverage helpers
- Updated scene and coverage tests

## Done when
Scene culling and coverage overlap no longer depend on ad-hoc geographic display math as their primary path.

## Verification
- Scene / coverage unit tests
- Targeted quilt-planner or scene-builder tests exercising projected overlap

## Notes
- Use one common display projection per frame / quilt plan.
- Do not move quilt policy into the host layer.
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
