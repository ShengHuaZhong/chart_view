# 88 - S-52 line style and area pattern engine

## Objective
Implement full line-style and area-pattern execution for the official compiled Phase 6 catalog.

## Phase
- Phase 6

## Layer
- runtime portrayal
- runtime renderer

## Depends on
- 87

## In scope
- Plain and symbolized boundaries
- Line symbol placement and styling
- Area fill, tiling, and edge handling
- Colour-table-driven line/area execution

## Out of scope
- No text engine work yet
- No projection or quilt-policy redesign
- No host changes

## Inputs
- official compiled catalog
- existing renderer path

## Required changes
- Add runtime line-style and area-pattern engines
- Integrate them into the S57 portrayal path
- Add focused boundary/pattern regression tests

## Deliverables
- Dedicated line/area engines
- Regression evidence for boundary and pattern behavior

## Done when
Line and area portrayal is driven by the official compiled catalog with meaningful coverage of boundary and fill behavior that exceeds the Phase 5 baseline.

## Verification
- Build line/area engine tests and affected smoke targets
- Run focused line-style and pattern regression coverage

## Notes
- Keep quilting and projection responsibilities where they already belong in runtime.
