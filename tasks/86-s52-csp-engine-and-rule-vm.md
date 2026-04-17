# 86 - S-52 CSP engine and rule VM

## Objective
Upgrade conditional symbology from the Phase 5 baseline into an official-asset-driven CSP/rule VM with explainable outputs.

## Phase
- Phase 6

## Layer
- runtime portrayal

## Depends on
- 85

## In scope
- Add CSP/rule VM execution over official compiled metadata
- Carry mariner settings, SCAMIN, palette, boundary mode, and safety context into evaluation
- Emit typed portrayal instructions and explain traces

## Out of scope
- No dedicated point engine yet
- No host changes
- No broad API redesign

## Inputs
- official compiled catalog
- existing conditional symbology baseline
- existing runtime inspection/query path

## Required changes
- Add the Phase 6 rule-VM execution layer
- Integrate official CSP metadata
- Extend inspection/explain surfaces only as narrowly as needed

## Deliverables
- Rule VM
- CSP execution coverage
- Explain/regression tests

## Done when
Conditional portrayal is driven by official compiled metadata instead of the earlier limited baseline logic, with focused explainable outputs.

## Verification
- Build the CSP/rule-VM targets
- Run focused Annex-A/S-64-inspired CSP regression tests

## Notes
- Prefer extending the existing runtime query/describe path over inventing a parallel debug API.
