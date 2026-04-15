# 30 — Quilt plan model

## Objective
Represent multi-chart composition plan

## Phase
- Phase 2

## Layer
- runtime

## Depends on
- 29

## In scope
- Define QuiltPlan, QuiltLayer, QuiltSelectionResult

## Out of scope
- No QWidget or RHI types

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Define QuiltPlan, QuiltLayer, QuiltSelectionResult

## Deliverables
- Quilt plan headers

## Done when
- Can represent selected charts for current frame

## Verification
- Compile-only + unit smoke

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
