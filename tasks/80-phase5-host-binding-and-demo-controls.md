# 80 - Phase 5 host binding and demo controls

## Objective
Bind the new Phase 5 runtime controls into the Qt host layers without moving rule logic out of `chart_runtime`.

## Phase
- Phase 5

## Layer
- qtwidgets
- standalone
- verification

## Depends on
- 79

## In scope
- Add host controls for mariner settings and selected filters
- Keep host code as a bridge/container only

## Out of scope
- No host-owned portrayal logic
- No parser / projection / renderer core migration into UI layers

## Inputs
- AGENTS.md
- docs/phase_roadmap.md
- tasks/79-full-s57-real-chart-smoke.md

## Required changes
- Add Qt Widgets / standalone bindings to the runtime API
- Add focused host smoke coverage

## Deliverables
- Demo-host control bindings
- Host smoke verification

## Done when
The demo host can drive Phase 5 runtime settings and filters while the logic remains runtime-owned.

## Verification
- Build `chart_qtwidgets` / `chart_standalone`
- Run focused host smoke tests

## Notes
- Preserve DLL-first boundaries.
