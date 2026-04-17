# 106 - S-52 upstream supplemental OpenCPN assets

## Objective
Vendor GPL-compatible supplemental OpenCPN resource files that fill the remaining
asset IDs before any manual overlay work begins.

## Phase
- Phase 6D

## Layer
- runtime
- portrayal
- tests
- docs

## Depends on
- 105

## In scope
- vendored supplemental OpenCPN resource files and provenance manifests
- overlay search-order changes inside runtime-internal compiler/asset loaders
- focused parser/compiler/asset regressions for the supplemental resources

## Out of scope
- No manual overlay asset creation
- No host changes
- No public ABI changes

## Required changes
- Search and vendor supplemental upstream resources for these IDs first:
  - `BOYLAT52`
  - `BOYLAT53`
  - `BOYLAT54`
  - `BOYLAT55`
  - `BOYLAT56`
  - `BOYSPP50`
  - `VEHTRF01`
  - `BCNCON81`
  - `ARCSLN01`
  - `DANGER53`
  - `BOYSPR02`
  - `BOYSPR03`
  - `NEWOBJ01` only if a real upstream asset exists
- Keep provenance for every added upstream resource
- Narrow the remaining missing-asset list to the residual manual-overlay set

## Done when
- the supplemental upstream pack is vendored with provenance
- compiler/asset loaders prefer pinned base snapshot first, then supplemental pack
- the targeted upstream asset IDs no longer fail only because they were absent from
  the base snapshot

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
