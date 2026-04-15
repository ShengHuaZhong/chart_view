# 03 — Qt Widgets DLL skeleton

## Objective
Minimal chart_qtwidgets shared library

## Phase
- Phase 1

## Layer
- qtwidgets

## Depends on
- 01
- 02

## In scope
- Add ChartViewWidget skeleton
- Add runtime bridge/controller skeleton

## Out of scope
- No chart parsing or SENC here

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add ChartViewWidget skeleton
- Add runtime bridge/controller skeleton

## Deliverables
- chart_qtwidgets target builds

## Done when
- Widget can be instantiated and attached to a runtime handle

## Verification
- Build widget smoke host

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
