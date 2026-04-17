# 109 - S-52 harness and standalone proof

## Objective
Expand repository-owned graphical evidence and direct host evidence until the
lookup-row baseline no longer carries harness-only partials.

## Phase
- Phase 6D

## Layer
- runtime
- qtwidgets
- standalone
- tests
- docs

## Depends on
- 108

## In scope
- fixed-scene reference expansion
- real-chart family metrics and host-proof notes
- direct standalone host evidence for the newly covered lookup-row families

## Out of scope
- No public ABI changes
- No moving runtime logic into the host

## Done when
- `scene_harness_not_covered` is no longer present in the committed inventory
- the new lookup-row families have repository-owned fixed-scene evidence
- direct standalone host evidence is recorded on a real S57 file

## Verification
- Build the relevant harness and host targets
- Run the relevant graphical/reference tests
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
