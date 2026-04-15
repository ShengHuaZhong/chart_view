# 26 — Phase 1 demo verification

## Objective
Close Phase 1 with all single-chart paths passing

## Phase
- Phase 1

## Layer
- verification

## Depends on
- 23
- 24
- 25

## In scope
- Document final Phase 1 verification matrix
- Confirm runtime/qtwidgets/standalone linkage and display path

## Out of scope
- No Phase 2 work

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Document final Phase 1 verification matrix
- Confirm runtime/qtwidgets/standalone linkage and display path

## Deliverables
- Verification note and summary

## Done when
- at least one canonical local S57 chart passes the full phase-1 path:
  source -> normalize -> SENC -> scene -> runtime render -> Qt host display
- at least one canonical local CM93 chart passes the same full path
- S-101 remains scaffolded/optional unless local test data is provided

## Verification
- Run all Phase 1 smokes

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
