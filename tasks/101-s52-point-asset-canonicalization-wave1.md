# 101 - S-52 point-asset canonicalization wave 1

## Objective
Canonicalize the remaining wave-1 point-symbol asset ids in the compiler so the compiled catalog resolves the known OpenCPN snapshot asset-family gaps without widening the runtime ABI.

## Phase
- Phase 6C wave 1

## Layer
- runtime
- portrayal
- tests

## Depends on
- 100

## In scope
- Compiler-stage point-asset id normalization helpers
- Wave-1 point-asset coverage for `FLTHAZ02`, `BOYLAT52/53/54/55/56`, `BOYSPP50`, `VEHTRF01`, `RDOCAL02/03`, `ESSARE01`, and `PSSARE01`
- Focused point-symbol regressions and inventory refresh

## Out of scope
- No host changes
- No public ABI changes
- No new reference-harness scenes

## Done when
- Wave-1 point-asset reasons disappear from the inventory or remain only for confirmed snapshot-missing assets
- `runtime.s52_catalog_compiler`, `runtime.feature_symbolizer`, `runtime.point_symbol`, and `runtime.feature_renderer` pass
