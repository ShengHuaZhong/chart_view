# 04 — Standalone host skeleton

## Objective
Minimal QMainWindow reference host

## Phase
- Phase 1

## Layer
- standalone

## Depends on
- 03

## In scope
- Create chart_standalone executable
- MainWindow with ChartViewWidget as central widget
- Basic menu/status bar scaffold

## Out of scope
- No runtime core logic in MainWindow

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Create chart_standalone executable
- MainWindow with ChartViewWidget as central widget
- Basic menu/status bar scaffold

## Deliverables
- main.cpp
- main_window.h/.cpp

## Done when
- Application starts and shows the central widget

## Verification
- Manual launch smoke

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
