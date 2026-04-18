# 106 - S-52 supplemental-route closure and backlog routing

## Objective
Close the original supplemental OpenCPN resource route with an explicit audit-backed
scope decision, and route the remaining IDs into the correct follow-on tasks
without widening scope or silently hiding backlog.

## Phase
- Phase 6D

## Layer
- docs
- state

## Depends on
- 105

## In scope
- task and state-file truth sync for the audited supplemental-resource conclusion
- explicit routing of residual IDs into:
  - `107-s52-manual-overlay-asset-pack`
  - `107a-inland-current-symbols-vehtrf01`
  - deferred legacy inland bucket
  - residual compatibility bucket
- docs updates that make the new task boundaries explicit

## Out of scope
- No runtime/compiler/renderer implementation changes
- No host changes
- No public ABI changes

## Required changes
- Record that the original supplemental-upstream sweep did not find honest
  vendorable OpenCPN resource definitions for the audited target IDs.
- Route the audited IDs as follows:
  - `ARCSLN01` -> `107-s52-manual-overlay-asset-pack`
  - `NEWOBJ01` -> `107-s52-manual-overlay-asset-pack`
  - `VEHTRF01` -> `107a-inland-current-symbols-vehtrf01`
  - `BOYLAT52`
  - `BOYLAT53`
  - `BOYLAT54`
  - `BOYLAT55`
  - `BOYLAT56`
  - `BOYSPP50`
    -> deferred legacy inland bucket pending product-positioning
  - `BCNCON81`
  - `DANGER53`
  - `BOYSPR02`
  - `BOYSPR03`
    -> residual compatibility IDs only
- Clear the active blocker created by the earlier, broader interpretation of task 106
  and restate the next implementation target without marking any follow-on task complete.

## Done when
- task 106 is explicitly defined as supplemental-route closure plus backlog routing
- the repository truth files and state files no longer claim that task 106 must
  vendor the audited IDs from OpenCPN upstream resources
- the next implementation tasks are unambiguous:
  - `107` for `ARCSLN01` and `NEWOBJ01`
  - `107a` for `VEHTRF01`
  - legacy inland remains deferred
  - residual compatibility IDs remain explicit and not silently counted as complete

## Verification
- Validate the rewritten task/docs/state text for consistency
- Run the minimum repo hygiene verification needed for the doc/state edits

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
  - build when code changes require it
  - relevant tests when code changes require them
  - direct standalone host run with a real S57 file when runtime or host behavior changes
- report:
  - the consistency of task, plan, roadmap, and state wording
  - whether task 106 remains an active blocker after the rewrite
- update:
  - state/current_iteration.md
  - state/done.md
  - or state/blocked.md if truly blocked
