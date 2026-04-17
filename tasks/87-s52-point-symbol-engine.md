# 87 - S-52 point symbol engine

## Objective
Introduce a dedicated point-symbol execution engine driven by official compiled symbol data.

## Phase
- Phase 6

## Layer
- runtime portrayal
- runtime renderer

## Depends on
- 86

## In scope
- Point symbol placement, pivoting, orientation, colour references, and composite symbol handling
- Official-asset-driven buoy/beacon/topmark/light portrayal

## Out of scope
- No line/area engine work yet
- No text engine work yet
- No host changes

## Inputs
- official compiled symbol catalog
- CSP/rule-VM outputs

## Required changes
- Add the runtime point-symbol engine
- Integrate point instruction execution into the normal S57 portrayal path
- Add focused regression tests for rotation/composition and key object families

## Deliverables
- Point-symbol runtime engine
- Regression coverage for major point-symbol behaviors

## Done when
Point features are rendered primarily through the official compiled symbol engine rather than the earlier simplified fallback path.

## Verification
- Build point-symbol engine tests and the affected renderer smoke targets
- Run the focused point-symbol regression suite

## Notes
- Keep the implementation runtime-owned; the host must remain a consumer only.
