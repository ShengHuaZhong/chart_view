# 78 - OpenCPN parity harness and reference samples

## Objective
Create an engineering parity harness that compares selected Phase 5 S57 behavior against OpenCPN and reference samples without treating OpenCPN as the normative source.

## Phase
- Phase 5

## Layer
- verification
- runtime

## Depends on
- 77

## In scope
- Define the parity harness inputs and selected sample charts
- Record structured comparison output such as rule hits or screenshot diffs

## Out of scope
- No code copying from OpenCPN
- No compliance claim

## Inputs
- AGENTS.md
- docs/phase_roadmap.md
- tasks/77-s57-class-and-rule-selection-controls.md

## Required changes
- Add harness scripts/tests/docs for parity comparison
- Keep normative-source wording clear

## Deliverables
- Engineering parity harness
- Reference-sample documentation

## Done when
The repository has a repeatable way to compare selected Phase 5 S57 behavior against OpenCPN for engineering cross-checks only.

## Verification
- Run the focused parity harness on the chosen sample set

## Notes
- Normative truth remains IHO S-52 / Annex A / S-64.
