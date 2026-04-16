# 75 - Full S52 renderer integration for S57

## Objective
Switch the S57 rendering path from the Phase 4 baseline subset to the fuller Phase 5 S-52 instruction execution path.

## Phase
- Phase 5

## Layer
- runtime
- portrayal
- render_core
- verification

## Depends on
- 74

## In scope
- Execute compiled point / line / area / text instructions in the runtime render path
- Reduce generic semantic fallback to a diagnostic/last-resort role

## Out of scope
- No host/UI logic
- No broad S-101 expansion

## Inputs
- AGENTS.md
- docs/phase_roadmap.md
- tasks/74-complete-conditional-symbology-engine.md

## Required changes
- Integrate full instruction execution into runtime renderers
- Add focused renderer tests and smoke coverage

## Deliverables
- Fuller S57 S-52 renderer integration
- Renderer regression coverage

## Done when
The normal S57 render path executes the compiled S-52 instruction set instead of depending on the Phase 3 generic fallback as the primary result.

## Verification
- Build feature-renderer and symbolized-smoke tests
- Run focused S57 renderer regression coverage

## Notes
- Preserve DLL-first and runtime-owned portrayal boundaries.
