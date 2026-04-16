# 79 - Full S57 real-chart smoke

## Objective
Validate the fuller Phase 5 S57 pipeline on a broader real-chart sample set.

## Phase
- Phase 5

## Layer
- verification
- runtime

## Depends on
- 78

## In scope
- Add broader real-chart smoke coverage
- Preserve the fixed pair `C1511781.000` / `C1511782.000`
- Add additional harbor / approach / coastal / overview samples as available

## Out of scope
- No compliance declaration
- No host-owned validation path

## Inputs
- AGENTS.md
- docs/phase_roadmap.md
- tasks/78-opencpn-parity-harness-and-reference-samples.md

## Required changes
- Add wider real-chart smoke coverage for the Phase 5 S57 path
- Document sample requirements and gating behavior honestly

## Deliverables
- Real-chart smoke tests
- Real-chart verification notes

## Done when
The fuller Phase 5 S57 path passes real-chart smoke coverage beyond the narrow fixed pair baseline.

## Verification
- Build the real-chart smoke targets
- Run the configured real-chart sample matrix

## Notes
- Keep skip/gating behavior honest when required assets are missing.
