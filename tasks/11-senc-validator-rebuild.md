# 11 — SENC validator and rebuild policy

## Objective
Detect when SENC must be rebuilt

## Phase
- Phase 1

## Layer
- senc

## Depends on
- 09
- 10

## In scope
- Implement SourceManifest comparison
- Implement rebuild-needed decision

## Out of scope
- No directory scanning yet

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Implement SourceManifest comparison
- Implement rebuild-needed decision

## Deliverables
- SencValidator sources

## Done when
- Source change can trigger rebuild-needed result

## Verification
- Unit tests for timestamp/hash changes

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
