# 22 — Open-chart routing

## Objective
Open file -> build/load SENC -> display

## Phase
- Phase 1

## Layer
- standalone

## Depends on
- 15
- 16
- 17
- 21

## In scope
- Add open-chart command in MainWindow
- Route by file type to proper reader/build/load flow

## Out of scope
- Do not move chart logic into MainWindow; call runtime-facing APIs

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add open-chart command in MainWindow
- Route by file type to proper reader/build/load flow

## Deliverables
- Host command routing

## Done when
- User can open one S57/CM93/S-101 file and reach display path

## Verification
- Manual/automated open-chart smoke

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
