# 32 — Scene builder from quilt

## Objective
Build MultiChart SceneSnapshot

## Phase
- Phase 2

## Layer
- runtime

## Depends on
- 31

## In scope
- Upgrade scene builder to consume QuiltPlan and multiple SENCs

## Out of scope
- Do not expand all charts globally

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Upgrade scene builder to consume QuiltPlan and multiple SENCs

## Deliverables
- Multi-chart scene builder

## Done when
- One frame can contain visible objects from multiple charts

## Verification
- Multi-chart scene smoke

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
