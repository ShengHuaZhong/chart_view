# 57 — S-52 presentation assets adapter

## Objective
Adapt S-52 presentation assets into runtime-owned data structures

## Phase
- Phase 4

## Layer
- portrayal / assets

## Depends on
- 56

## In scope
- Introduce runtime-owned structures for S-52 colors, symbols, line styles, and area patterns
- Add a narrow adapter or importer layer for the S-52 assets used by the baseline
- Keep the current portrayal registry as a runtime-owned concept while adding an S-52-backed asset source

## Out of scope
- No full lookup engine yet
- No conditional symbology yet
- No Unicode text work yet

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add S-52 asset adapter sources
- Add asset-loading or asset-query tests
- Document the normative-source boundary: IHO assets/rules are the source of truth, OpenCPN is reference only

## Deliverables
- S-52 asset adapter sources
- Asset query tests

## Done when
The runtime can query a baseline set of S-52 presentation assets without hardcoding every asset directly in renderer code.

## Verification
- S-52 asset adapter unit tests

## Notes
- Do not copy OpenCPN code or asset packaging directly.
- Keep S57 as the primary Phase 4 target.
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
