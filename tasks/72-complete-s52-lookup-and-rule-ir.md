# 72 - Complete S52 lookup and rule IR

## Objective
Replace the current narrow hand-written lookup subset with a compiled S-52 lookup model and typed rule/instruction IR.

## Phase
- Phase 5

## Layer
- runtime
- portrayal
- verification

## Depends on
- 71

## In scope
- Add compiled lookup rows and typed instruction IR
- Preserve display category, priority, view group, and stable rule ids
- Keep lookup ownership inside `chart_runtime`

## Out of scope
- No public rule-filter API yet
- No host logic

## Inputs
- AGENTS.md
- docs/phase_roadmap.md
- tasks/71-private-s52-source-catalog-compiler.md

## Required changes
- Introduce typed point / line / area / text / conditional instruction representations
- Route symbolization through compiled lookup data
- Add focused rule-hit tests

## Deliverables
- Compiled lookup model
- Instruction IR
- Lookup verification coverage

## Done when
The runtime can match S57 features against the compiled S-52 lookup catalog and emit stable typed rule instructions.

## Verification
- Build lookup / symbolizer tests
- Run compiled rule-hit coverage

## Notes
- Keep renderer execution separate from lookup generation.
