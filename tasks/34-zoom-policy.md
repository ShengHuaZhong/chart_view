# 34 — Zoom policy

## Objective
Explicit scale behavior and chart re-selection triggers

## Phase
- Phase 2

## Layer
- runtime

## Depends on
- 31

## In scope
- Implement ZoomPolicy
- Define overzoom/underzoom rules and rebuild triggers

## Out of scope
- No UI event handling here

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Implement ZoomPolicy
- Define overzoom/underzoom rules and rebuild triggers

## Deliverables
- Zoom policy sources

## Done when
- Scale behavior is deterministic and testable

## Verification
- Zoom policy unit tests

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
