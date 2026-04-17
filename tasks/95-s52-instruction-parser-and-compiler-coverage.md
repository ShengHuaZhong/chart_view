# 95 - S-52 instruction parser and compiler coverage

## Objective
Expand the `chartsymbols.xml` parser/compiler path so unsupported or partially supported instruction metadata from the Phase 6B inventory enters `chart_view`'s internal IR instead of being silently dropped.

## Phase
- Phase 6B

## Layer
- portrayal
- runtime
- tests

## Depends on
- 94

## In scope
- Parser/compiler metadata coverage expansions
- Explicit unsupported-token reporting
- Focused parser/compiler regression tests

## Out of scope
- No renderer changes yet
- No host changes
- No public ABI changes
