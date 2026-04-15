# 29 — Chart selection policy

## Objective
Rank candidate charts for current viewport scale

## Phase
- Phase 2

## Layer
- runtime

## Depends on
- 28

## In scope
- Implement scale/usage/source priority rules

## Out of scope
- No UI logic

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Implement scale/usage/source priority rules

## Deliverables
- Selection policy sources

## Done when
- Given candidates and scale, return stable ordered chart list

## Verification
- Policy unit tests

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
