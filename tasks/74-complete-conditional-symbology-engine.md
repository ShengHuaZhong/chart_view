# 74 - Complete conditional symbology engine

## Objective
Expand the conditional-symbology baseline into a fuller runtime-owned S-52 conditional engine for S57.

## Phase
- Phase 5

## Layer
- runtime
- portrayal
- verification

## Depends on
- 73

## In scope
- Implement the selected Phase 5 conditional-symbology set
- Honor display category, SCAMIN, safety settings, lights, and boundary variants as required by the chosen baseline

## Out of scope
- No host-owned rule logic
- No task 75 renderer execution yet beyond condition outputs

## Inputs
- AGENTS.md
- docs/phase_roadmap.md
- tasks/73-full-mariner-settings-runtime-api.md

## Required changes
- Extend the runtime conditional engine
- Add focused rule-behavior tests tied to mariner settings

## Deliverables
- Runtime-owned conditional symbology engine
- Conditional behavior test coverage

## Done when
The runtime can evaluate the chosen Phase 5 S-52 conditional behaviors inside the portrayal pipeline.

## Verification
- Build conditional-symbology tests
- Run focused settings-driven behavior coverage

## Notes
- Keep the normative-source boundary explicit.
