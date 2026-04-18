# Phase 6D Task 107a Inland-Current Manual Overlay

Task `107a-inland-current-symbols-vehtrf01` is intentionally narrow.

It adds a repo-owned runtime-internal manual overlay asset only for:

- `VEHTRF01`

Scope boundaries:

- `VEHTRF01` is treated as an inland-current compatibility symbol, not as an
  automatic extension of the ordinary S-52 Presentation Library manual-overlay path
- `ARCSLN01` and `NEWOBJ01` remain task-107 assets
- `BOYLAT52/53/54/55/56` and `BOYSPP50` remain deferred legacy inland symbols
- `BCNCON81`, `DANGER53`, `BOYSPR02`, and `BOYSPR03` remain residual
  compatibility IDs only

Implementation note:

- The runtime keeps this overlay internal to the compiler/asset-loader path.
- No runtime public ABI changes are introduced.
- The overlay is applied only on the OpenCPN fallback catalog path.

Provenance note:

- See `vendor/manual_s52_overlay/phase6d_task107a/PROVENANCE.manifest`.
