# 06 — RHI backend bootstrap

## Objective
Minimal Qt 6 RHI render backbone

## Phase
- Phase 1

## Layer
- render_core

## Depends on
- 02
- 05

## In scope
- Add RhiRenderBackend
- Create minimal frame lifecycle and clear pass

## Out of scope
- No parsing logic in render core

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add RhiRenderBackend
- Create minimal frame lifecycle and clear pass

## Deliverables
- render_core sources inside chart_runtime

## Done when
- Runtime can execute a minimal render frame path

## Verification
- Build + minimal render smoke

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
