# 35 — ChartView zoom interaction

## Objective
Bind Qt zoom input to viewport and quilt recompute

## Phase
- Phase 2

## Layer
- qtwidgets

## Depends on
- 34
- 33

## In scope
- Wheel zoom, anchor zoom, viewport updates, render requests

## Out of scope
- Do not put selection policy in widget layer

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Wheel zoom, anchor zoom, viewport updates, render requests

## Deliverables
- ChartViewWidget interaction updates

## Done when
- Zooming triggers viewport + quilt + render update without jumps

## Verification
- Manual/integration zoom smoke

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
