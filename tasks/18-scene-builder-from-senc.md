# 18 — Scene builder from SENC

## Objective
Build SceneSnapshot from SENC for one viewport

## Phase
- Phase 1

## Layer
- runtime

## Depends on
- 10
- 15
- 16
- 17

## In scope
- Implement SceneBuilderFromSenc
- Use SpatialIndex and RenderCache to select visible objects

## Out of scope
- Do not consume raw source readers at render time

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Implement SceneBuilderFromSenc
- Use SpatialIndex and RenderCache to select visible objects

## Deliverables
- Scene builder sources

## Done when
- Given viewport + SENC, can build visible scene snapshot

## Verification
- Scene build unit smoke

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
