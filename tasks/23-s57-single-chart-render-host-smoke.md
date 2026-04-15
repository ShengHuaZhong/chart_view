# 23 — S57 single-chart host render smoke

## Objective
Show one S57 chart in standalone host

## Phase
- Phase 1

## Layer
- verification

## Depends on
- 22

## In scope
- Add S57 host smoke path

## Out of scope
- No Phase 2 quilting

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md
- canonical local S57 fixture
- generated or cached SENC
## Required changes
- Add S57 host smoke path

## Deliverables
- s57 host smoke test

## Done when
- Standalone host shows non-blank S57 content and logs extent/feature count

## Verification
- Run integration smoke
- open canonical S57 through the normal host path
- build or load SENC
- render non-blank content in the Qt host
- no crash on initial display
## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
