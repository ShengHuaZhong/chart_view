# 114 - e4.0.0 alias and compatibility audit

## Objective
Audit and route the non-direct official-e4.0.0 asset set without inventing
unsupported official assets.

## Phase
- Phase 6 official-asset follow-up

## Layer
- runtime
- portrayal
- tests
- docs
- state

## Depends on
- 113

## In scope
- alias / provenance audit for:
  - `BOYSPH79`
  - `TOPSHP33`
- explicit routing audit for:
  - `VEHTRF01`
  - `BOYLAT52`
  - `BOYLAT53`
  - `BOYLAT54`
  - `BOYLAT55`
  - `BOYLAT56`
  - `BOYSPP50`
  - `BCNCON81`
  - `DANGER53`
  - `BOYSPR02`
  - `BOYSPR03`
- explicit mainline-vs-inland-vs-legacy-vs-residual documentation

## Out of scope
- No host changes
- No runtime public ABI changes
- Do not reopen:
  - `ARCSLN01`
  - `NEWOBJ01`
- Do not re-route `VEHTRF01` back into the ordinary maritime mainline
- Do not invent an official asset for `BOYSPH79` or `TOPSHP33` before the audit
  proves the provenance

## Required changes
- Keep the routing rules explicit:
  - `FLTHAZ02` / `ESSARE01` / `PSSARE01`: official wave-1 maritime mainline
  - `ARCSLN01` / `NEWOBJ01`: existing repo-owned overlays, do not reopen
  - `VEHTRF01`: inland-current
  - `BOYLAT52-56` / `BOYSPP50`: legacy inland
  - `BCNCON81` / `DANGER53` / `BOYSPR02` / `BOYSPR03`: residual compatibility
  - `BOYSPH79` / `TOPSHP33`: alias / provenance audit first

## Done when
- the audit records an explicit route for each listed ID without mixing the
  buckets together
- unresolved IDs remain explicit as blockers or quarantined compatibility
  buckets
- no host changes or public-ABI changes were required

## Verification
- Build the relevant audit / inventory targets
- Run the relevant focused tests or scripts
- Run a direct standalone host smoke with a real S57 file when the active
  runtime path is affected

## Mandatory constraints
Read and follow these files first, in this order:

1. `AGENTS.md`
2. `state/current_iteration.md`
3. `state/blocked.md`
4. `state/done.md`
5. `docs/phase6d_all_lookuprow_verification.md`
6. `docs/generated/phase6d_all_lookuprow_verification_bucket_summary.json`
7. `src/runtime/portrayal/opencpn_chartsymbols_parser.*`
8. `src/runtime/portrayal/s52_source_catalog_compiler.*`
9. `src/runtime/portrayal/s52_lookup_model.*`
10. `src/runtime/portrayal/s52_presentation_assets.*`
11. `src/runtime/portrayal/s52_conditional_opcode.hpp`
12. `src/runtime/portrayal/s52_conditional_symbology.*`
13. `src/runtime/portrayal/feature_symbolizer.*`
14. relevant S-52 tests / reference harness

After code changes:
- run the minimum verification needed to prove the fix
- include:
  - build
  - relevant tests
  - direct standalone host run with a real S57 file when the task changes the
    active runtime path
- report:
  - whether the window is still blank or now visibly non-blank
  - whether resize preserves the viewport center
  - what presentation path is now used
- update:
  - `state/current_iteration.md`
  - `state/done.md`
  - or `state/blocked.md` if truly blocked
