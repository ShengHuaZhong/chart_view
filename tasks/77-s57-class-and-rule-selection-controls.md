# 77 - S57 class and rule selection controls

## Objective
Make the Phase 5 runtime honor object-class and stable rule-level selection controls for S57 portrayal.

## Phase
- Phase 5

## Layer
- runtime
- portrayal
- verification

## Depends on
- 76

## In scope
- Apply object-class filters
- Apply stable rule-id filters
- Keep filtering precedence runtime-owned

## Out of scope
- No host UI binding yet
- No unrelated portrayal expansion

## Inputs
- AGENTS.md
- docs/phase_roadmap.md
- tasks/76-s57-query-inspection-and-rule-explain.md

## Required changes
- Wire filter settings into the runtime portrayal pipeline
- Add focused filter-precedence tests

## Deliverables
- Runtime object/rule filtering behavior
- Verification coverage for filter precedence

## Done when
The runtime can selectively suppress S57 symbol content by object class and stable rule id without host-owned rule logic.

## Verification
- Build filter-behavior tests
- Run focused object/rule filtering coverage

## Notes
- Stable rule ids must come from the compiled S-52 catalog, not transient renderer state.
