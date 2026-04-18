# 110 - Phase 6D all lookuprow verification

## Objective
Close the Phase 6D chain with a final verification note that proves the committed
lookup-row baseline is fully supported.

## Phase
- Phase 6D

## Layer
- runtime
- tests
- docs
- state

## Depends on
- 109

## In scope
- final Phase 6D verification note
- focused verification matrix rerun
- state-file closeout for the task chain

## Out of scope
- No new runtime features
- No host changes
- No public ABI changes

## Done when
- the final note records:
  - `lookupRowsTotal = 3057`
  - `supportedRows = 3057`
  - `partialRows = 0`
  - `unsupportedRows = 0`
- the note records which resources came from:
  - the pinned base snapshot
  - the supplemental OpenCPN pack
  - the manual overlay pack
- 必须同时给出 ordinary S57 rows 与 internal/meta rows 的分离统计
## Verification
- Build the focused Phase 6D matrix
- Run the focused tests needed to prove the final baseline
- Run a direct standalone host smoke with a real S57 file
- 必须分别验证 ordinary 和 internal/meta 两边都归零

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
