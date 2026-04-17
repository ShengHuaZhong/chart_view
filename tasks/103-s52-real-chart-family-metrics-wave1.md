# 103 - S-52 real-chart family metrics wave 1

## Objective
Extend the existing real-chart lookup-coverage smoke so the Phase 6C wave-1 object families report explicit compiled/fallback/text-hit metrics on the retained three-chart sample set.

## Phase
- Phase 6C wave 1

## Layer
- runtime
- tests
- docs

## Depends on
- 102

## In scope
- New wave-1 family buckets in `runtime.s57_lookup_coverage_smoke`
- Documentation of missing real-chart exposure where a family is only covered by fixed scenes
- Verification on `C1511781`, `C1511782`, and `C1511783`

## Out of scope
- No host changes
- No public ABI changes
- No new chart-sample selection logic

## Done when
- Wave-1 family metrics are emitted
- For families with `seen > 0`, `preferredCompiledHits > 0` and `fallbackHits == 0`
- `runtime.s57_lookup_coverage_smoke` and `runtime.s57_real_chart_smoke` pass
