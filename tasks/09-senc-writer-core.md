# 09 — SENC writer core

## Objective
Write SENC v1 files

## Phase
- Phase 1

## Layer
- senc

## Depends on
- 08

## In scope
- Implement SencWriter
- Implement ordered section writing
- Support empty/minimal datasets

## Out of scope
- No format-specific parsing here

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Implement SencWriter
- Implement ordered section writing
- Support empty/minimal datasets

## Deliverables
- SencWriter sources

## Done when
- Can write a structurally valid SENC file

## Verification
- Roundtrip smoke with synthetic dataset

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
