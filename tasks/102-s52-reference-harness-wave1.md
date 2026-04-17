# 102 - S-52 reference harness wave 1

## Objective
Add repository-owned fixed-scene graphical evidence for the Phase 6C wave-1 object families so their compiled rows and assets are proven by committed references rather than inventory counts alone.

## Phase
- Phase 6C wave 1

## Layer
- tests
- docs
- reference data

## Depends on
- 101

## In scope
- Three new wave-1 fixed scenes
- Committed reference JSON and crop specs
- Object-level assertions for wave-1 rule/source/asset coverage

## Out of scope
- No host changes
- No public ABI changes
- No new real-chart dataset-selection logic

## Done when
- `runtime.chart1_s64_reference_harness` replays the three new wave-1 scenes
- `runtime.s64_reference_smoke` remains green
