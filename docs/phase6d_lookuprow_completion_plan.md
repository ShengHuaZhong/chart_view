# Phase 6D Lookup-Row Completion Plan

Task `105-phase6d-truth-sync-and-task-chain` starts the next lookup-row completion
chain after the Phase 6B and Phase 6C wave-1 baselines.

## Goal

Complete all remaining lookup rows on the approved `chartsymbols.xml` fallback path
without widening the runtime ABI, changing host ownership, or linking `s52plib`.

The Phase 6D success target is:

- `lookupRowsTotal = 3057`
- `supportedRows = 3057`
- `partialRows = 0`
- `unsupportedRows = 0`

## Resource resolution order

Phase 6D uses this fixed resource order:

1. existing assets already present in
   `vendor/opencpn_s57data/Release_5.14.0/s57data`
2. GPL-compatible vendored supplemental OpenCPN resource files and atlases
3. repo-owned manual overlay assets for residual gaps only

## OpenCPN supplemental resources first

The first supplemental upstream sweep should try to source these asset IDs from
additional OpenCPN resource files before any manual drawing work:

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
- `NEWOBJ01` only if a real upstream asset exists instead of parser/compiler noise

## Manual overlay second

Only after the supplemental upstream sweep is complete should the repository add
manual overlay assets for residual gaps such as:

- `FLTHAZ02`
- `BOYSPH79`
- `ESSARE01`
- `PSSARE01`

Task `107` is the first manual-overlay task and should keep provenance notes for
every asset it creates.

## Parser/compiler cleanup stays separate

The following malformed spillover IDs are not the first manual-overlay targets and
should be fixed through parser/compiler cleanup instead:

- `TOPSHP73TESOBJNAM...`
- `DGPS01DRFSTA01`
- `TOWERS74TXOBJNAM...`
- similarly malformed combined asset/text strings

## Task chain

- `105-phase6d-truth-sync-and-task-chain`
- `106-s52-upstream-supplemental-opencpn-assets`
- `107-s52-manual-overlay-asset-pack`
- `108-s52-all-lookuprow-closure`
- `109-s52-harness-and-standalone-proof`
- `110-phase6d-all-lookuprow-verification`
