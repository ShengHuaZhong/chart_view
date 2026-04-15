# 20 — Runtime render frame API

## Objective
Expose render driving through runtime API

## Phase
- Phase 1

## Layer
- runtime

## Depends on
- 18
- 19

## In scope
- Add runtime API for set viewport / load SENC / render frame

## Out of scope
- No QWidget in public API

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add runtime API for set viewport / load SENC / render frame

## Deliverables
- Runtime C API additions

## Done when
- External host can drive one frame through runtime API

## Verification
- Runtime smoke with render frame call

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
