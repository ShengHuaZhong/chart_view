# 41 — Portrayal registry core

## Objective
Central registry for symbol/style rules

## Phase
- Phase 3

## Layer
- portrayal

## Depends on
- 40

## In scope
- Add PortrayalRegistry
- Add SymbolRule / LineStyleRule / AreaFillRule / TextRule

## Out of scope
- No full catalogue import yet

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add PortrayalRegistry
- Add SymbolRule / LineStyleRule / AreaFillRule / TextRule

## Deliverables
- Portrayal registry sources

## Done when
- Renderer no longer hardcodes all styles directly

## Verification
- Registry unit tests

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
