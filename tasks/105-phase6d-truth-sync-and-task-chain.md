# 105 - Phase 6D truth sync and task chain

## Objective
Sync repository truth to the post-Phase-6C state and land the Phase 6D lookup-row
completion chain, with supplemental OpenCPN resources first and manual overlay
assets second.

## Phase
- Phase 6D

## Layer
- docs
- planning
- tasks
- state

## Depends on
- 104

## In scope
- `AGENTS.md`, `plan.md`, and roadmap truth sync
- new Phase 6D task files
- state-file updates for the new active chain
- a narrow Phase 6D planning note describing supplemental-vs-manual asset order

## Out of scope
- No runtime parser/compiler behavior changes
- No host changes
- No public ABI changes
- No task-106+ implementation

## Done when
- `AGENTS.md`, `plan.md`, and `docs/phase_roadmap.md` point to the Phase 6D chain
- task files `105-110` exist
- each new task file carries the required mandatory-constraint block
- `state/current_iteration.md` advances to task `106`

## Verification
- Build `chart_standalone` and `qtwidgets_smoke_tests`
- Run the Qt Widgets smoke to keep resize/non-blank evidence alive
- Run a direct standalone host smoke with a real S57 file

## Mandatory constraints
Read and follow these files first, in this order:

1. AGENTS.md
2. plan.md
3. docs/architecture.md
4. docs/build_environment.md
5. docs/coding_rules.md
6. state/done.md
7. state/current_iteration.md
8. state/blocked.md

After code changes:
- run the minimum verification needed to prove the fix
- include:
  - build
  - relevant tests
  - direct standalone host run with a real S57 file
- report:
  - whether the window is still blank or now visibly non-blank
  - whether resize preserves the viewport center
  - what presentation path is now used
- update:
  - state/current_iteration.md
  - state/done.md
  - or state/blocked.md if truly blocked
