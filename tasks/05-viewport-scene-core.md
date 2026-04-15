# 05 — Viewport + scene core

## Objective
Explicit viewport and scene snapshot model

## Phase
- Phase 1

## Layer
- runtime

## Depends on
- 02

## In scope
- Add ViewportState
- Add SceneModel and SceneSnapshot
- Add RenderScheduler skeleton

## Out of scope
- No widget or shell state in these classes

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add ViewportState
- Add SceneModel and SceneSnapshot
- Add RenderScheduler skeleton

## Deliverables
- Core model headers/sources

## Done when
- Runtime can store and update viewport without QWidget dependencies

## Verification
- Build runtime smoke

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
