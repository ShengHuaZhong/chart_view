# 107 - S-52 manual overlay asset pack

## Objective
Create the repo-owned manual overlay pack for the two confirmed S-52 Presentation
Library compatibility symbols that should be hand-implemented on the main S-52 path:

- `ARCSLN01`
- `NEWOBJ01`

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
- manual overlay assets for `ARCSLN01` and `NEWOBJ01` only
- provenance and reason notes for those manual assets
- compiler/asset-loader support for those two overlay assets
- focused point/line symbol proof for those two IDs

## Out of scope
- No host changes
- No public ABI changes
- No hiding unresolved ordinary S57 rows by changing inventory accounting
- No `VEHTRF01`
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

## Required changes
- Implement `ARCSLN01` as a repo-owned manual overlay symbol asset using the
  available S-52 Presentation Library evidence.
- Implement `NEWOBJ01` as a repo-owned manual overlay symbol asset using the
  available S-52 Presentation Library evidence.
- Keep provenance, source note, and reason-for-manual metadata for both IDs.
- Ensure the compiler/asset-loader path can resolve these two IDs without changing
  the runtime public ABI or relying on OpenCPN supplemental upstream resources.

## Done when
- `ARCSLN01` no longer remains partial only because its symbol asset is missing
- `NEWOBJ01` no longer remains partial only because its symbol asset is missing
- both manual assets have provenance, source note, and reason-for-manual metadata

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
