# 91 - Chart 1 and S-64 reference harness

## Objective
Replace counter-only validation as the main evidence with Chart 1 and selected S-64 graphical reference harnesses.

## Phase
- Phase 6

## Layer
- verification
- tooling

## Depends on
- 90

## In scope
- Fixed-scene reference harnesses for Chart 1 and selected S-64 cases
- Screenshot/tile diff support
- Object-level rule/instruction assertions
- Observation/reference manifests under the hybrid artifact policy

## Out of scope
- No OpenCPN comparison yet
- No compliance claim
- No host redesign

## Inputs
- vendored Chart 1 assets
- selected S-64 scenes
- completed Phase 6 portrayal path

## Required changes
- Add the reference harness scripts/tests
- Commit focused goldens/crops/manifests
- Add repeatable scene definitions and assertions

## Deliverables
- Chart 1 reference harness
- S-64 selected-scene reference harness
- Focused goldens and manifests

## Done when
Phase 6 verification has reproducible graphical and object-level evidence beyond smoke counters for Chart 1 and selected S-64 scenes.

## Verification
- Build the new reference-harness targets
- Run the Chart 1 and S-64 harness commands end to end

## Notes
- From this task onward, counter-only smoke evidence is not enough as the main acceptance proof.
