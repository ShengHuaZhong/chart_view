# 27 — Chart catalog core

## Objective
Load SENC metadata for multiple charts

## Phase
- Phase 2

## Layer
- runtime

## Depends on
- 26

## In scope
- Define ChartCatalogEntry and ChartCatalog
- Load chart metadata from SENC directory without full geometry

## Out of scope
- No rendering logic here

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Define ChartCatalogEntry and ChartCatalog
- Load chart metadata from SENC directory without full geometry

## Deliverables
- Catalog sources

## Done when
- Can list charts by id/extent/nativeScale/sourceType

## Verification
- Catalog unit smoke

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
