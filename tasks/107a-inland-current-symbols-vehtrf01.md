# 107a - Inland current symbols: VEHTRF01

## Objective
Create the repo-owned manual symbol asset needed for `VEHTRF01`, which belongs to
the current inland ECDIS / ES-RIS compatibility scope rather than the main S-52
Presentation Library symbol set.

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
- manual overlay asset for `VEHTRF01` only
- provenance and source note for `VEHTRF01`
- compiler/asset-loader support needed to resolve `VEHTRF01`
- minimal graphical/reference proof for `VEHTRF01`

## Out of scope
- No host changes
- No public ABI changes
- No legacy inland symbols:
  - `BOYLAT52`
  - `BOYLAT53`
  - `BOYLAT54`
  - `BOYLAT55`
  - `BOYLAT56`
  - `BOYSPP50`
- No residual compatibility IDs:
  - `BCNCON81`
  - `DANGER53`
  - `BOYSPR02`
  - `BOYSPR03`
- No ordinary S-52 Presentation Library manual-overlay work beyond `VEHTRF01`

## Required changes
- Implement `VEHTRF01` as a repo-owned manual symbol asset.
- Keep provenance, source note, and reason-for-manual metadata for `VEHTRF01`.
- Treat `VEHTRF01` as an inland-current compatibility task, not as an automatic
  expansion of task `107`.

## Done when
- `VEHTRF01` no longer remains partial only because its symbol asset is missing
- the task records that `VEHTRF01` was resolved as an inland-current compatibility
  symbol rather than a mainline S-52 PL symbol
- the repository contains minimal graphical proof for `VEHTRF01`

## Mainline note
- This task does not block the narrow S-52 Presentation Library symbol-closure
  work in task `107`.
- It does block any final claim that all lookup rows are fully complete if
  `VEHTRF01` remains unresolved.

## Verification
- Build the relevant compiler/asset/renderer targets
- Run focused parser/compiler/point/line tests
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
