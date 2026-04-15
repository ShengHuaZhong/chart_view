# 01 — Runtime API contract

## Objective
Define narrow runtime ABI

## Phase
- Phase 1

## Layer
- runtime

## Depends on
- 00

## In scope
- Create interfaces/runtime/chart_runtime_c_api.h
- Create interfaces/runtime/chart_runtime_types.h
- Define opaque handle and lifecycle API

## Out of scope
- No QWidget/QRhi in public ABI

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Create interfaces/runtime/chart_runtime_c_api.h
- Create interfaces/runtime/chart_runtime_types.h
- Define opaque handle and lifecycle API

## Deliverables
- Public runtime headers

## Done when
- Headers compile in isolation
- API is host-neutral

## Verification
- Compile public headers in a tiny smoke target

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
