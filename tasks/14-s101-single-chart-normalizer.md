# 14 — S-101 normalizer

## Objective
Single S-101 chart to unified dataset

## Phase
- Phase 1

## Layer
- chart_data

## Depends on
- 07

## In scope
- Add S101Reader
- Add S101Normalizer

## Out of scope
- No full portrayal catalogue support yet

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add S101Reader
- Add S101Normalizer

## Deliverables
- s101 reader/normalizer sources

## Done when
- Outputs FeatureChartDataset

## Verification
- Reader smoke on one sample chart

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
- S-101 real-data verification is optional in the current repository state.
This task should complete only the model/path scaffolding unless S-101 local data is later provided.