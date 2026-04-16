# 58 — S-52 lookup and instruction model

## Objective
S-52 lookup layer and runtime instruction model for S57

## Phase
- Phase 4

## Layer
- portrayal / symbolizer

## Depends on
- 57

## In scope
- Replace selected generic style-key decisions with an explicit S-52 instruction model for S57 objects
- Add lookup logic from feature + attributes + context to render instructions
- Preserve the DLL-first portrayal boundary by keeping lookups in `chart_runtime`

## Out of scope
- No display-mode toggles yet
- No conditional symbology yet
- No font fallback yet

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add S-52 instruction DTOs / internal structs
- Upgrade `FeatureSymbolizer` or equivalent runtime ownership to emit S-52 instructions for selected S57 classes
- Add lookup-focused tests

## Deliverables
- S-52 instruction model
- Lookup tests for selected S57 classes

## Done when
Selected S57 objects resolve through an explicit S-52 instruction path instead of only generic semantic style keys.

## Verification
- Feature-symbolizer tests
- S57 lookup / instruction tests

## Notes
- This task changes decision-making, not final renderer execution details.
- Keep the public ABI narrow.
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
