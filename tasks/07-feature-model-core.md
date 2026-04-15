# 07 — Unified feature model

## Objective
Common internal feature model for S57/CM93/S-101

## Phase
- Phase 1

## Layer
- chart_data

## Depends on
- 05

## In scope
- Add FeatureChartDataset
- Add point/line/area geometry types
- Add DatasetMeta and minimal attributes

## Out of scope
- Do not create per-format renderer-specific models

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add FeatureChartDataset
- Add point/line/area geometry types
- Add DatasetMeta and minimal attributes

## Deliverables
- Feature model headers/sources

## Done when
- Three input formats can map into the same model

## Verification
- Unit test with synthetic data

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
