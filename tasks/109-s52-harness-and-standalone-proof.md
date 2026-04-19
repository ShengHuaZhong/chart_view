# 109 - Phase 6D workstream B: CSP engine closure

## Objective
Advance the runtime-owned CSP engine from the current legacy / partial state
toward the active portrayal semantics set, without widening the runtime ABI or
moving logic into the host.

## Phase
- Phase 6D

## Layer
- runtime
- portrayal
- tests
- docs

## Depends on
- 108

## In scope
- CSP/opcode inventory for the current runtime-owned engine
- focused behavior closure for high-value portrayal chains:
  - depth / safety / sounding
  - obstruction / wreck
  - restriction / lights
  - `NEWOBJ`
- clear separation between lookup/CSP behavior and non-lookup free-text behavior
- focused unit/smoke regressions for the updated CSP paths

## Out of scope
- No public ABI changes
- No moving runtime logic into the host
- No new manual asset sweep
- No repository-owned harness expansion beyond the minimum needed to prove the CSP fix

## Done when
- the focused CSP families owned by this workstream are no longer legacy/no-op
  placeholders on the active runtime path
- the new or corrected CSP behavior is covered by focused tests or smokes
- the task records the current lookup/CSP versus free-text instruction boundary

## Verification
- Build the relevant compiler/CSP/symbolizer targets
- Run the relevant focused tests or smokes
- Run a direct standalone host smoke with a real S57 file

## Mandatory constraints
Read and follow these files first, in this order:

1. AGENTS.md
2. plan.md
3. docs/architecture.md
4. docs/build_environment.md
5. docs/coding_rules.md
6. state/done.md
7. state/current_iteration.md
8. state/blocked.md

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
  - state/current_iteration.md
  - state/done.md
  - or state/blocked.md if truly blocked
