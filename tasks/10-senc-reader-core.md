# 10 — SENC reader core

## Objective
Read SENC v1 files

## Phase
- Phase 1

## Layer
- senc

## Depends on
- 08
- 09

## In scope
- Implement SencReader
- Validate header/version
- Locate sections

## Out of scope
- No scene build yet

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Implement SencReader
- Validate header/version
- Locate sections

## Deliverables
- SencReader sources

## Done when
- Can read back SENC written by task 09

## Verification
- Roundtrip smoke test

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
