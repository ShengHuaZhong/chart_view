# 28 — Coverage index

## Objective
Spatial lookup for chart coverages

## Phase
- Phase 2

## Layer
- runtime

## Depends on
- 27

## In scope
- Build CoverageIndex over chart extents

## Out of scope
- No renderer coupling

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Build CoverageIndex over chart extents

## Deliverables
- Coverage index sources

## Done when
- Given viewport bbox, return intersecting charts efficiently

## Verification
- Coverage query smoke

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
