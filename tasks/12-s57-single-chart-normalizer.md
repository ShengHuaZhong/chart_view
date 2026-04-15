# 12 — S57 normalizer

## Objective
Single S57 chart to unified dataset

## Phase
- Phase 1

## Layer
- chart_data

## Depends on
- 07
- 11a-chart-testdata-fixtures

## In scope
- Add S57Reader
- Add S57Normalizer
- Support single .000 input

## Out of scope
- No full update-cell merge optimization

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md
- local canonical S57 fixture from `CHARTSYS_S57_TESTDATA_ROOT`

## Required changes
- Add S57Reader
- Add S57Normalizer
- Support single .000 input

## Deliverables
- s57 reader/normalizer sources

## Done when
- can read the canonical local S57 chart
- can output a `FeatureChartDataset`
- point / line / area categories are extracted
- feature count is non-zero
- dataset extent is valid

## Verification
- Reader smoke on one sample chart
- load the canonical S57 source chart
- normalize into `FeatureChartDataset`
- validate point / line / area extraction
- validate non-zero feature count
- validate dataset extent

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.



