# 112 - e4.0.0 compiler / loader integration

## Objective
Connect the official e4.0.0 DAI-derived `S52SourceCatalog` into the compiler,
loader, and preferred-catalog path while keeping the OpenCPN fallback path
available.

## Phase
- Phase 6 official-asset follow-up

## Layer
- runtime
- portrayal
- tests
- state

## Depends on
- 111

## In scope
- preferred-catalog and loader integration for the DAI-derived source catalog
- official-versus-fallback source precedence at the runtime-internal loader
  boundary
- explicit provenance and edition metadata propagation through the official
  ingest path
- focused compiler / lookup regressions for the official preferred-catalog path

## Out of scope
- No host changes
- No runtime public ABI changes
- No official static-asset wave expansion beyond what the DAI already provides
- No alias-routing audit for inland / residual symbols
- No Chart 1 / S-64 baseline expansion

## Required changes
- Add an official DAI-based source-catalog loader path alongside the existing
  OpenCPN fallback loader path.
- Keep resource precedence explicit:
  1. local official DAI path
  2. pinned OpenCPN fallback path
  3. repo-owned manual overlays
- Preserve the current fallback path for regression / emergency fallback.

## Done when
- the runtime can compile and load the official DAI-derived catalog through the
  preferred official path
- the current fallback OpenCPN path still exists as an explicit fallback
- focused compiler / lookup tests prove the official path is usable without host
  changes or public-ABI expansion

## Verification
- Build the relevant compiler / lookup / symbolizer targets
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
