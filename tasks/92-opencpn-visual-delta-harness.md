# 92 - OpenCPN visual delta harness

## Objective
Add a visual-delta and object-statistics comparison harness against OpenCPN for engineering analysis only.

## Phase
- Phase 6

## Layer
- verification
- tooling

## Depends on
- 91

## In scope
- Visual-delta comparisons on fixed charts, viewports, and settings
- Object/rule statistics delta reporting
- Screenshot-crop diff summaries

## Out of scope
- No normative pass/fail based on OpenCPN
- No code copying
- No host redesign

## Inputs
- task 91 fixed-scene definitions
- curated OpenCPN captures or manifests
- completed Phase 6 portrayal path

## Required changes
- Extend or replace the Phase 5 parity harness with graphical delta support
- Keep the same fixed-scene inputs used by the normative harnesses
- Add docs explaining how to interpret delta results

## Deliverables
- OpenCPN visual-delta harness
- Engineering delta reports

## Done when
The repository can compare its Phase 6 output with curated OpenCPN observations on fixed scenes, while keeping IHO sources as the normative truth.

## Verification
- Run the OpenCPN visual-delta harness on the selected fixed scenes

## Notes
- This harness is for engineering comparison only.
