# 59 — S-52 display settings and conditional symbology

## Objective
Runtime-owned S-52 display settings and conditional symbolization baseline

## Phase
- Phase 4

## Layer
- portrayal / settings / CSP

## Depends on
- 58

## In scope
- Add runtime-owned display settings for the Phase 4 S57 baseline
- Add a baseline conditional-symbology evaluator for selected key S57 object families
- Make S-52 instruction results depend on settings / context where required

## Out of scope
- No full type-approval scope
- No multilingual text work yet
- No host-owned settings UI

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add narrow runtime DTOs or internal settings models as needed without exposing PROJ or deep internals
- Add conditional-symbolization logic for a selected baseline subset
- Add settings / conditional tests

## Deliverables
- S-52 settings baseline
- Conditional-symbolization tests

## Done when
Selected S57 instruction outputs change correctly when S-52 display settings or conditional rules require it.

## Verification
- Settings tests
- Conditional-symbolization tests
- Updated symbolizer or portrayal tests

## Notes
- Stay honest about baseline scope; do not call this full compliance.
- Keep the host as a settings caller only, not the logic owner.
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
