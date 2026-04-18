# Phase 6D Task 107 Manual Overlay Assets

Task `107-s52-manual-overlay-asset-pack` is intentionally narrow.

It adds repo-owned manual overlay assets only for:

- `ARCSLN01`
- `NEWOBJ01`

Scope boundaries:

- `VEHTRF01` is explicitly excluded and belongs to `107a-inland-current-symbols-vehtrf01`
- `BOYLAT52/53/54/55/56` and `BOYSPP50` remain deferred legacy inland symbols
- `BCNCON81`, `DANGER53`, `BOYSPR02`, and `BOYSPR03` remain residual compatibility IDs only

Implementation note:

- The runtime keeps these overlays internal to the compiler/asset-loader path.
- No runtime public ABI changes are introduced.
- The overlays are applied only on the OpenCPN fallback catalog path.

Provenance note:

- See `vendor/manual_s52_overlay/phase6d_task107/PROVENANCE.manifest`.
