# 53 — PROJ projection context core

## Objective
Runtime-owned PROJ projection context for display-space transforms

## Phase
- Phase 4

## Layer
- projection / runtime

## Depends on
- 52

## In scope
- Add a runtime-internal `ProjectionContext` that owns PROJ setup for one display CRS
- Add display-space project / unproject helpers and batch point-transform helpers
- Keep PROJ handles private to `chart_runtime`
- Integrate PROJ as a runtime dependency without widening the public ABI
- Add projection-focused unit tests

## Out of scope
- No quilt patch clipping yet
- No S-52 portrayal logic yet
- No Unicode / multilingual text work yet
- No host-side projection ownership

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add runtime-internal PROJ wrapper sources
- Add projected viewport / display CRS helpers
- Wire build configuration for PROJ in the runtime target only
- Add focused projection tests

## Deliverables
- PROJ-backed projection context sources
- Projection unit tests

## Done when
The runtime can transform chart-space lon/lat into one explicit display projection without exposing PROJ through the public ABI.

## Verification
- Projection unit tests
- Targeted runtime build proving the new PROJ dependency stays inside `chart_runtime`

## Notes
- This task establishes the Phase 4 projection foundation.
- State what this task will not modify before editing.
- Preserve DLL-first boundaries.
- Update `state/current_iteration.md` and `state/done.md` after completion.
