# 13 — CM93 normalizer

## Objective
Single CM93 chart to unified dataset

## Phase
- Phase 1

## Layer
- chart_data

## Depends on
- 07
- 11a-chart-testdata-fixtures

## In scope
- Add Cm93Reader
- Add Cm93Normalizer

## Out of scope
- Do not create a separate render model

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md
- local canonical CM93 fixture from `CHARTSYS_CM93_TESTDATA_ROOT`

## Required changes
- Add Cm93Reader
- Add Cm93Normalizer

## Deliverables
- cm93 reader/normalizer sources

## Done when
- Outputs FeatureChartDataset

## Verification
- Reader smoke on one sample chart
- load the canonical CM93 chart
- normalize into `FeatureChartDataset`
- validate non-zero feature count
- validate dataset extent

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
