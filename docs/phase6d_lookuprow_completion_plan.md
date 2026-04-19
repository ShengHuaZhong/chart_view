# Phase 6D Lookup-Row Completion Plan

Task `105-phase6d-truth-sync-and-task-chain` starts the next lookup-row completion
chain after the Phase 6B and Phase 6C wave-1 baselines.

## Goal

Complete all remaining lookup rows on the approved `chartsymbols.xml` fallback path
without widening the runtime ABI, changing host ownership, or linking `s52plib`.

The Phase 6D execution note now uses four explicit workstreams:

- Workstream A:
  - IR compiler normalization and alias closure
  - owner: `108-s52-all-lookuprow-closure`
- Workstream B:
  - CSP engine closure
  - owner: `109-s52-harness-and-standalone-proof`
- Workstream C:
  - asset completion
  - owners:
    - `107-s52-manual-overlay-asset-pack`
    - `107a-inland-current-symbols-vehtrf01`
- Workstream D:
  - verification baseline
  - owner: `110-phase6d-all-lookuprow-verification`

The final verification policy must report the ordinary maritime, inland-current,
legacy inland, residual compatibility, and internal/meta buckets separately
instead of hiding them behind a single completion count.

## Resource resolution order

Phase 6D uses this fixed resource order:

1. existing assets already present in
   `vendor/opencpn_s57data/Release_5.14.0/s57data`
2. GPL-compatible vendored supplemental OpenCPN resource files and atlases
3. repo-owned manual overlay assets for residual gaps only

## Supplemental-route closure first

The original supplemental-upstream sweep audited these IDs first:

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
- `NEWOBJ01`

The current audited conclusion is:

- no honest vendorable OpenCPN supplemental resource definitions were confirmed for
  those IDs in the pinned snapshot or the sampled upstream tags
- task `106` therefore acts as a provenance split / route-closure note and routes
  the IDs into explicit follow-on buckets instead of continuing to block on a
  resource pack that has not been found

## Asset workstream

Task `107` is the narrow maritime confirmed manual-overlay task for:

- `ARCSLN01`
- `NEWOBJ01`

Task `107a` is the separate inland-current compatibility task for:

- `VEHTRF01`

These IDs are deferred and are not part of task `107`:

- `BOYLAT52`
- `BOYLAT53`
- `BOYLAT54`
- `BOYLAT55`
- `BOYLAT56`
- `BOYSPP50`

These IDs remain residual compatibility markers only until stronger evidence is
available:

- `BCNCON81`
- `DANGER53`
- `BOYSPR02`
- `BOYSPR03`

Additional residual gaps that remain on the ordinary maritime S-52 path may still
require later manual overlays, such as:

- `FLTHAZ02`
- `BOYSPH79`
- `ESSARE01`
- `PSSARE01`

Any such follow-on manual overlays should be added only after the narrow `107`
and `107a` scopes are complete and still keep provenance notes for every asset.

## Workstream A - IR compiler cleanup stays separate

The following malformed spillover IDs are not manual-overlay-first targets and
should be fixed through compiler/parser normalization instead:

- `TOPSHP73TESOBJNAM...`
- `DGPS01DRFSTA01`
- `TOWERS74TXOBJNAM...`
- similarly malformed combined asset/text strings

## Task chain

- `105-phase6d-truth-sync-and-task-chain`
- `106-s52-upstream-supplemental-opencpn-assets`
- `107-s52-manual-overlay-asset-pack`
- `107a-inland-current-symbols-vehtrf01`
- `108-s52-all-lookuprow-closure`
- `109-s52-harness-and-standalone-proof`
- `110-phase6d-all-lookuprow-verification`

See [phase6d_execution_workstreams.md](C:\Users\zsh\source\repos\chart_view\docs\phase6d_execution_workstreams.md)
for the explicit mapping and bucket boundaries.
