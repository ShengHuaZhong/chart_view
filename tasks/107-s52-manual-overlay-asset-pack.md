# 107 - S-52 manual overlay asset pack

## Objective
Create the repo-owned manual overlay pack for residual lookup-row asset gaps that
remain absent after the supplemental OpenCPN sweep.

## Phase
- Phase 6D

## Layer
- runtime
- portrayal
- tests
- docs

## Depends on
- 106

## In scope
- manual overlay asset package
- provenance and reason notes for each manual asset
- compiler/asset-loader support for the manual overlay tier

## Out of scope
- No host changes
- No public ABI changes
- No hiding unresolved ordinary S57 rows by changing inventory accounting

## Required changes
- Reserve manual overlay for residual gaps such as:
  - `FLTHAZ02`
  - `BOYSPH79`
  - `ESSARE01`
  - `PSSARE01`
- Only add more manual overlay assets if task `106` proves they are still absent
  after the supplemental upstream sweep

## Done when
- the residual missing ordinary S57 asset IDs have a repo-owned manual overlay
  asset or an explicit blocker
- every manual asset has provenance, source note, and reason-for-manual metadata

## Verification
- Build the relevant compiler/asset/renderer targets
- Run focused parser/compiler/point/line/area tests
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
