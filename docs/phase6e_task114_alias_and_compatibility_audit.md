# Phase 6E Task 114: Alias and Compatibility Audit

Task `114-e400-alias-and-compatibility-audit` keeps the non-direct official
e4.0.0 asset set explicit without inventing unsupported official assets.

This task does not change host architecture, does not widen the runtime public
ABI, and does not reopen already-closed overlays such as `ARCSLN01` and
`NEWOBJ01`.

## Inputs used

- official narrative and machine-readable references, local and read-only:
  - `docs/reference_local/PresLib_e4.0.0.dai`
  - `docs/reference_local/S-52_PresLib_e4.0.0_Part_I_Clean_Draft.pdf`
- fallback engineering reference:
  - `vendor/opencpn_s57data/Release_5.14.0/s57data/chartsymbols.xml`

The official DAI is the normative truth source for this audit. The OpenCPN
snapshot is used only as fallback engineering evidence.

## Audit result

The following IDs do **not** have a direct official e4.0.0 DAI definition but
do appear in the fallback snapshot:

- `BOYSPH79`
- `TOPSHP33`
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

Nearby official e4.0.0 candidates do exist for the alias/provenance subset:

- `BOYSPH01`
- `TOPMAR33`

Task 114 keeps those relationships explicit but **does not** promote them into
official aliases automatically.

## Routing outcome

- official maritime mainline remains:
  - `FLTHAZ02`
  - `ESSARE01`
  - `PSSARE01`
- existing repo-owned overlays remain:
  - `ARCSLN01`
  - `NEWOBJ01`
- inland-current stays separate:
  - `VEHTRF01`
- legacy inland stays deferred:
  - `BOYLAT52`
  - `BOYLAT53`
  - `BOYLAT54`
  - `BOYLAT55`
  - `BOYLAT56`
  - `BOYSPP50`
- residual compatibility stays quarantined:
  - `BCNCON81`
  - `DANGER53`
  - `BOYSPR02`
  - `BOYSPR03`
- alias / provenance audit remains explicit:
  - `BOYSPH79`
  - `TOPSHP33`

## Machine-readable record

The explicit routing table is committed in:

- `docs/generated/phase6e_task114_alias_and_compatibility_routes.json`

## Verification

Task 114 verification stays narrow because it does not change the active
runtime path:

- focused audit/build tests
- `qtwidgets.smoke`
- direct standalone real-S57 smoke for continuity reporting

The expected visible/runtime status remains unchanged:

- window remains visibly non-blank
- resize-center behavior remains covered by `qtwidgets.smoke`
- presentation path remains:
  - `chart_runtime renderFrame -> copyFrameRgba -> QImage -> QPainter::drawImage`
