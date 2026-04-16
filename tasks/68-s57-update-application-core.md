# 68 - S57 update application core

## Objective
Apply `.001+` ENC updates onto the new internal S57 source model inside `chart_runtime`.

## Phase
- Phase 5

## Layer
- runtime
- chart_data
- verification

## Depends on
- 67

## In scope
- Parse sequential update files for a base ENC
- Apply updates to the internal source model before dataset derivation
- Track applied-update state and failure cases

## Out of scope
- No SENC v2 format yet
- No host-owned update logic
- No portrayal / renderer changes

## Inputs
- AGENTS.md
- docs/phase_roadmap.md
- docs/architecture.md
- tasks/67-s57-source-model-and-update-manifest.md

## Required changes
- Add update-application logic inside `src/runtime/s57/*`
- Define update application ordering and rejection rules
- Cover synthetic base + update sequences in tests

## Deliverables
- Runtime-owned update application core
- Tests for sequential and missing-update cases

## Done when
The runtime can apply a synthetic base ENC plus `.001+` updates into one authoritative internal S57 source model.

## Verification
- Build the focused S57 reader/update tests
- Run the new runtime update-application tests

## Notes
- Keep update application inside the runtime ingest/build path.
