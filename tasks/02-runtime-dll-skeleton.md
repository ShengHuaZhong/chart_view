# 02 — Runtime DLL skeleton

## Objective
Minimal chart_runtime shared library

## Phase
- Phase 1

## Layer
- runtime

## Depends on
- 01

## In scope
- Implement create/initialize/shutdown runtime skeleton
- Add minimal internal runtime context

## Out of scope
- No chart parsing yet beyond scaffolding

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Implement create/initialize/shutdown runtime skeleton
- Add minimal internal runtime context

## Deliverables
- chart_runtime target builds

## Done when
- External smoke target can create/init/shutdown runtime

## Verification
- Build runtime_smoke

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
