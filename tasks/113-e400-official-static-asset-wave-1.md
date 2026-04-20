# 113 - e4.0.0 official static asset wave 1

## Objective
Close the first official static-asset wave on the DAI-driven path for:
- `FLTHAZ02`
- `ESSARE01`
- `PSSARE01`

## Phase
- Phase 6 official-asset follow-up

## Layer
- runtime
- portrayal
- tests
- docs
- state

## Depends on
- 112

## In scope
- official e4.0.0 mainline closure for:
  - `FLTHAZ02`
  - `ESSARE01`
  - `PSSARE01`
- compiler / loader / presentation-asset closure needed to expose those assets
  on the runtime-owned mainline
- focused graphical or asset-level regressions proving the official path is
  active for those symbols

## Out of scope
- Do not reopen repo-owned overlays for:
  - `ARCSLN01`
  - `NEWOBJ01`
- Do not pull inland-current or legacy inland assets into this task
- No host changes
- No runtime public ABI changes

## Required changes
- Close the official-e4.0.0 path for the wave-1 static assets.
- Keep the already-existing repo-owned overlays intact without reopening them.
- Preserve explicit routing boundaries:
  - `VEHTRF01` stays inland-current
  - `BOYLAT52-56` and `BOYSPP50` stay legacy inland
  - `BCNCON81`, `DANGER53`, `BOYSPR02`, `BOYSPR03` stay residual compatibility
  - `BOYSPH79`, `TOPSHP33` stay alias / provenance audit until proven

## Done when
- the three wave-1 official assets are available through the official mainline
  path or the task records an honest blocker
- focused tests prove the official path exposes those assets on the runtime
  side
- no host changes or public-ABI changes were required

## Verification
- Build the relevant compiler / asset / renderer targets
- Run the relevant focused tests
- Run a direct standalone host smoke with a real S57 file

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
  - direct standalone host run with a real S57 file
- report:
  - whether the window is still blank or now visibly non-blank
  - whether resize preserves the viewport center
  - what presentation path is now used
- update:
  - `state/current_iteration.md`
  - `state/done.md`
  - or `state/blocked.md` if truly blocked
