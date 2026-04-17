# 82 - Repo truth sync for Phase 6

## Objective
Synchronize the repository truth files with the real post-Phase-5 state and create the formal Phase 6 task chain.

## Phase
- Phase 6

## Layer
- repo metadata
- docs
- task definitions

## Depends on
- 81

## In scope
- Update `AGENTS.md`, `plan.md`, and `docs/phase_roadmap.md` to reflect the completed Phase 5 baseline and the new Phase 6 direction
- Create task files `82-93`
- Record the Phase 6 truth-source, asset-source, and verification strategy boundaries

## Out of scope
- No runtime behavior changes
- No host changes
- No public ABI changes
- No official asset ingest yet

## Inputs
- `AGENTS.md`
- `plan.md`
- `docs/phase_roadmap.md`
- `state/current_iteration.md`
- `state/done.md`

## Required changes
- Sync the top-level repository guidance to the real current state
- Add Phase 6 roadmap language and guardrails
- Add task files `82-93`

## Deliverables
- Updated repo truth documents
- Formal Phase 6 task files

## Done when
The repository's top-level planning files no longer imply the repo stops at Phase 4 or Phase 5, and the Phase 6 task chain exists as concrete task files.

## Verification
- `git diff --check`
- `powershell -NoProfile -Command "Get-ChildItem tasks | Where-Object { $_.Name -match '^(82|83|84|85|86|87|88|89|90|91|92|93)-' } | Select-Object -ExpandProperty Name"`

## Notes
- This task exists to prevent future agents from being misled by stale planning documents.
