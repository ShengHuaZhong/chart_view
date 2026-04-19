# 110 - Phase 6D workstream D: verification baseline

## Objective
Close the active Phase 6D workstreams with a repository-owned verification
baseline that reports the ordinary maritime, inland-current, legacy inland,
residual compatibility, and internal/meta buckets explicitly.

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
- focused Phase 6D verification note
- focused build / test / reference-harness / real-chart / direct-standalone reruns
- explicit bucket reporting for:
  - ordinary maritime S57 rows
  - inland-current rows
  - legacy inland decision-gate rows
  - residual compatibility rows
  - internal/meta rows
- state-file closeout for the active Phase 6D workstreams

## Out of scope
- No new runtime features
- No host changes
- No public ABI changes
- No ECDIS compliance claim

## Done when
- the final note records the current committed lookup-row baseline with explicit
  per-bucket counts rather than a hidden aggregate
- the note records which resources or behaviors came from:
  - the pinned base snapshot
  - supplemental OpenCPN resources, if any
  - repo-owned manual overlays
  - compiler-owned normalization
  - runtime-owned CSP closure
- the verification note states clearly which buckets are complete, deferred,
  quarantined, or still blocked
- the note does not make an ECDIS compliance claim

## Verification
- Build the focused Phase 6D matrix
- Run the focused tests needed to prove the active workstream baseline
- Run a direct standalone host smoke with a real S57 file
- Report:
  - whether the window is still blank or now visibly non-blank
  - whether resize preserves the viewport center
  - what presentation path is now used

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
