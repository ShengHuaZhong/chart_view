# 60 — S-52 renderer integration for S57

## Objective
Execute S-52 instructions through the existing runtime renderers for S57

## Phase
- Phase 4

## Layer
- renderer / portrayal integration

## Depends on
- 59

## In scope
- Integrate the S-52 instruction model into the current point / line / area / text renderer pipeline
- Keep display priority and scene ownership inside the runtime
- Upgrade integrated S57 symbolized rendering from the Phase 3 baseline to an S-52-backed baseline

## Out of scope
- No Unicode font fallback yet
- No multilingual label selection yet
- No S-101 full portrayal expansion

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Update renderers to consume S-52 instructions
- Update integrated renderer tests and S57 symbolized smoke coverage
- Preserve the runtime-owned final frame path

## Deliverables
- S-52-backed renderer integration for S57
- Updated S57 symbolized smoke coverage

## Done when
The runtime renders selected S57 objects through S-52-backed instructions instead of only the generic Phase 3 semantic baseline.

## Verification
- Point / line / area / label renderer tests as needed
- Integrated S57 symbolized smoke tests

## Notes
- Do not move S-52 branching into widget or host code.
- Keep S57 first.
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
