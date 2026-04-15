# 08 — SENC section model

## Objective
Define SENC v1 binary layout

## Phase
- Phase 1

## Layer
- senc

## Depends on
- 07

## In scope
- Define FileHeader
- Define SectionDesc and section enum
- Define mandatory sections listed in docs/senc_v1_contract.md

## Out of scope
- No ad hoc partial SENC definition

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Define FileHeader
- Define SectionDesc and section enum
- Define mandatory sections listed in docs/senc_v1_contract.md

## Deliverables
- SENC header/section declarations

## Done when
- SENC section layout is fixed and versioned

## Verification
- Header-only compile + static layout checks

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
