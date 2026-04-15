# 19 — Feature renderer (RHI)

## Objective
Render point/line/area from scene snapshot

## Phase
- Phase 1

## Layer
- render_core

## Depends on
- 06
- 18

## In scope
- Implement FeatureLayerRenderer
- Support basic point/line/area rendering via RHI

## Out of scope
- No full nautical symbolization

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Implement FeatureLayerRenderer
- Support basic point/line/area rendering via RHI

## Deliverables
- Feature renderer sources

## Done when
- SceneSnapshot geometry becomes visible in render path

## Verification
- Single-frame render smoke

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
