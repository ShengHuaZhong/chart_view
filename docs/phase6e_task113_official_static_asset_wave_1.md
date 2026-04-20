# Phase 6E Task 113: Official Static Asset Wave 1

Task `113-e400-official-static-asset-wave-1` closes the first official
e4.0.0 DAI-driven static-asset wave on the runtime-owned mainline only.

Wave-1 official assets:
- `FLTHAZ02`
- `ESSARE01`
- `PSSARE01`

Current routing boundaries remain explicit:
- `ARCSLN01` / `NEWOBJ01`
  - stay on the already-existing repo-owned overlays
- `VEHTRF01`
  - stays inland-current
- `BOYLAT52-56` / `BOYSPP50`
  - stay legacy inland
- `BCNCON81` / `DANGER53` / `BOYSPR02` / `BOYSPR03`
  - stay residual compatibility
- `BOYSPH79` / `TOPSHP33`
  - stay alias / provenance audit until proven

This task does not widen the runtime public ABI and does not change host
architecture. The verification surface is intentionally runtime-owned:
- `S52PresentationAssets`
- `FeatureSymbolizer`
- point/line renderers
- direct standalone real-S57 smoke

The local official references remain read-only inputs only:
- `docs/reference_local/PresLib_e4.0.0.dai`
- `docs/reference_local/S-52_PresLib_e4.0.0_Part_I_Clean_Draft.pdf`
