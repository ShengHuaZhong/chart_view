# Testing Strategy

## Layers
- `tests/runtime/` — runtime-only smoke and unit tests
- `tests/integration/` — host integration and end-to-end smoke tests
- `tests/unit/` — pure model/policy/unit-level tests

## Phase 1 minimum tests
- runtime lifecycle smoke
- S57 SENC build/read smoke
- CM93 SENC build/read smoke
- S-101 SENC build/read smoke
- single-chart host render smoke for S57 / CM93 / S-101

## Phase 2 minimum tests
- chart catalog smoke
- coverage index smoke
- quilt planner smoke
- zoom policy smoke
- quilt render smoke for S57 / CM93 / S-101

## Phase 3 minimum tests
- portrayal registry smoke
- symbolizer smoke
- symbolized render smoke for S57 / S-101
