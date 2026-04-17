# Phase 6C Wave 1 Point-Asset Canonicalization

Task 101 keeps the Phase 6C wave-1 work inside the runtime portrayal/compiler boundary and focuses on wave-1 point-asset coverage only.

The task does two things:

1. It canonicalizes inventory-side point-asset references so lower-case or formatting-only variants such as `rdocal02` match the compiled point-symbol assets that already exist in the catalog.
2. It keeps geometry-only point symbols in the compiled catalog and presentation-asset registry even when the source snapshot omits an explicit `color-ref`.

That second rule matters for the current vendored OpenCPN snapshot because assets such as `TOPMAR90` and `TOPMAR93` provide bitmap/vector metrics that the renderer can still use with the symbol-rule fallback color.

Task 101 intentionally does **not** invent assets that are absent from the pinned snapshot. Rows that still reference missing point assets after canonicalization remain partial and keep an explicit `compiler_missing_point_asset:*` reason for follow-up work or later waves.

Observed task-101 result on the committed inventory baseline:

- wave-1 false gaps for `BOYWTW`, `RDOCAL`, `TOPMAR`, and `VEHTRF` are gone after compiler-side canonicalization and geometry-only point-symbol retention
- the remaining wave-1 point-asset partials are now the snapshot-backed gaps that task 101 intentionally does not invent:
  - `BOYLAT -> BOYSPH79`
  - `OBSTRN -> FLTHAZ02`
  - `RESARE -> ESSARE01`
  - `RESARE -> PSSARE01`
