# 21 — Qt ChartView widget

## Objective
Connect ChartViewWidget to runtime render loop

## Phase
- Phase 1

## Layer
- qtwidgets

## Depends on
- 03
- 20

## In scope
- Bind widget resize/repaint to runtime
- Maintain runtime controller inside qtwidgets layer

## Out of scope
- No reader/SENC logic here

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Bind widget resize/repaint to runtime
- Maintain runtime controller inside qtwidgets layer

## Deliverables
- ChartViewWidget implementation

## Done when
- Widget can display runtime output

## Verification
- Widget host smoke

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
