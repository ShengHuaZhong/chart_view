# 111 - e4.0.0 DAI parser and source-catalog bridge

## Objective
Implement a runtime-owned bridge from the local official
`PresLib_e4.0.0.dai` input into `chart_view`'s existing `S52SourceCatalog`
without changing the host architecture or widening the runtime public ABI.

## Phase
- Phase 6 official-asset follow-up

## Layer
- runtime
- portrayal
- tests
- state

## Depends on
- 110

## In scope
- parsing the local full initial-transfer `.dai` file into runtime-internal
  source-catalog structures
- ingest of:
  - `LBID`
  - `COLS` / `CCIE`
  - `LUPT` / `ATTC` / `INST` / `DISC` / `LUCM`
  - `PATT` / `PATD`
  - `SYMB` / `SYMD` / `SXPO` / `SCRF` / `SVCT`
  - `LNST` / `LIND` / `LXPO` / `LCRF` / `LVCT`
- runtime-internal provenance / edition metadata needed to preserve the DAI
  source identity
- focused tests proving the resulting catalog feeds
  `S52SourceCatalogCompiler::compile(...)`

## Out of scope
- No preferred-catalog switch
- No host changes
- No runtime public ABI changes
- No OpenCPN fallback removal
- No official wave-1 static asset completion beyond ingest
- No Chart 1 / S-64 verification expansion

## Required changes
- Add a runtime-owned DAI parser / bridge that reads the local
  `docs/reference_local/PresLib_e4.0.0.dai` file.
- Convert the supported DAI records into the existing `S52SourceCatalog`
  structures, extending those structures only internally if necessary.
- Keep the existing OpenCPN `chartsymbols.xml` path intact as a coexisting
  fallback path.
- Record official DAI provenance / edition metadata in the runtime-internal
  source catalog.

## Done when
- the local `.dai` file parses successfully
- the parser output converts into a valid `S52SourceCatalog`
- `S52SourceCatalogCompiler::compile(...)` can consume the bridged catalog and
  produce at least:
  - colors
  - point symbols
  - line styles
  - area patterns
  - lookup rows
- focused tests prove the DAI ingest works without host changes or public-ABI
  expansion

## Verification
- Build the relevant DAI parser / compiler / lookup targets
- Run:
  - `runtime.e400_dai_source_catalog_bridge`
  - `runtime.s52_catalog_compiler`
  - `runtime.s52_lookup_model`
- Run `git diff --check`

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
    active runtime path or the task explicitly requires host evidence
- report:
  - whether the window is still blank or now visibly non-blank
  - whether resize preserves the viewport center
  - what presentation path is now used
- update:
  - `state/current_iteration.md`
  - `state/done.md`
  - or `state/blocked.md` if truly blocked
