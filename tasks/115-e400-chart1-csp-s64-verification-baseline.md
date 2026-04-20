# 115 - e4.0.0 Chart 1 / CSP / S-64 verification baseline

## Objective
Establish the next official-input verification baseline for the e4.0.0 DAI path
across Chart 1, colour test, CSP, and S-64 evidence. If the required official
reference inputs are still missing locally, record an honest blocker instead of
claiming completion.

## Phase
- Phase 6 official-asset follow-up

## Layer
- runtime
- tests
- docs
- state

## Depends on
- 114

## In scope
- Chart 1 / colour test / CSP UML / S-64 baseline planning and evidence wiring
- focused verification matrix for the official-input path
- explicit blocker recording when the required official verification inputs are
  missing locally

## Out of scope
- No host changes
- No runtime public ABI changes
- No ECDIS compliance claim
- Do not claim final higher-edition closure from e4.0.0 alone

## Required changes
- Use locally available official verification inputs when present.
- If local inputs are missing, record the blocker explicitly for:
  - Chart 1 pseudo-S57 package
  - colour test S-57 package
  - CSP UML / Enterprise Architect package
  - S-64 test data
  - higher-edition official references if the repo target remains above e4.0.0

## Done when
- the repo either has a focused official-input verification baseline for the
  current post-110 chain
- or it records an honest blocker for the missing official verification inputs
- no host changes or public-ABI changes were required
- no ECDIS compliance claim is made

## Verification
- Build the focused official-input verification targets
- Run the relevant focused tests or harnesses
- Run a direct standalone host smoke with a real S57 file

## Mandatory constraints
Read and follow these files first, in this order:

1. `AGENTS.md`
2. `state/current_iteration.md`
3. `state/blocked.md`
4. `state/done.md`
5. `docs/phase6d_all_lookuprow_verification.md`
6. `docs/generated/phase6d_all_lookuprow_verification_bucket_summary.json`
7. `src/runtime/portrayal/opencpn_chartsymbols_parser.*`
8. `src/runtime/portrayal/s52_source_catalog_compiler.*`
9. `src/runtime/portrayal/s52_lookup_model.*`
10. `src/runtime/portrayal/s52_presentation_assets.*`
11. `src/runtime/portrayal/s52_conditional_opcode.hpp`
12. `src/runtime/portrayal/s52_conditional_symbology.*`
13. `src/runtime/portrayal/feature_symbolizer.*`
14. relevant S-52 tests / reference harness

After code changes:
- run the minimum verification needed to prove the fix
- include:
  - build
  - relevant tests
  - direct standalone host run with a real S57 file
- report:
  - whether the window is still blank or now visibly non-blank
  - whether resize preserves the viewport center
  - what presentation path is now used
- update:
  - `state/current_iteration.md`
  - `state/done.md`
  - or `state/blocked.md` if truly blocked
