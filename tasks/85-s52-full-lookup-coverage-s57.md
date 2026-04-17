# 85 - S-52 full lookup coverage for S57

## Objective
Expand S57 semantic retention and lookup matching so real S57 objects can reliably hit the official Phase 6 lookup coverage.

## Phase
- Phase 6

## Layer
- runtime
- S57 reader
- portrayal lookup

## Depends on
- 84

## In scope
- Build coverage inventories from official lookup data and real chart samples
- Expand S57 semantic retention for object/attribute combinations needed by the official lookup set
- Reduce real-chart lookup misses in a measured way

## Out of scope
- No CSP VM yet
- No point/line/area/text engine work yet
- No host changes

## Inputs
- official compiled lookup catalog
- real S57 sample data
- existing S57 reader and semantic model

## Required changes
- Generate lookup coverage inventories and miss reports
- Extend reader/semantic retention to support the missing official lookup keys
- Add focused regression tests on real or synthetic S57 coverage samples

## Deliverables
- Coverage inventory tooling/tests
- Fuller lookup-hit path for S57

## Done when
The repository has measurable lookup coverage inventory and the real S57 path hits the official catalog for a materially broader object/attribute set than the Phase 5 baseline.

## Verification
- Build the S57 reader, lookup-model, and new coverage-inventory targets
- Run the focused coverage regression suite

## Notes
- Do not blind-expand rules without inventory evidence.
