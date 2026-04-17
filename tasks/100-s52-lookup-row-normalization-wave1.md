# 100 - S-52 lookup-row normalization wave 1

## Objective
Normalize the source-row to compiled-row matching path for the Phase 6C wave-1 object families so rows that already compile into the OpenCPN-derived catalog are no longer reported as missing lookup rows.

## Phase
- Phase 6C wave 1

## Layer
- runtime
- portrayal
- tests
- docs

## Depends on
- 99

## In scope
- Source/compiled row-key normalization for the Phase 6C wave-1 families
- Inventory baseline refresh
- Focused inventory/lookup regression coverage
- State-file updates for the new Phase 6C chain

## Out of scope
- No point-asset canonicalization beyond what is strictly required for rows to compile
- No host changes
- No public ABI changes

## Done when
- Wave-1 families no longer report `compiler_missing_lookup_row`
- Updated inventory baseline is committed
- `runtime.s52_resource_snapshot_inventory`, `runtime.s52_catalog_compiler`, and `runtime.s52_lookup_model` pass
