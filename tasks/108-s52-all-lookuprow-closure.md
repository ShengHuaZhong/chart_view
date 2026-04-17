# 108 - S-52 all lookuprow closure

## Objective
Close the remaining lookup-row backlog so every row from the pinned fallback path
enters the compiled/runtime mainline.

## Phase
- Phase 6D

## Layer
- runtime
- portrayal
- tests
- docs

## Depends on
- 107

## In scope
- parser/compiler normalization for all remaining lookup rows
- overlay precedence and lookup-row closure
- focused inventory/compiler/lookup regressions

## Out of scope
- No host changes
- No public ABI changes
- No compliance claims

## Required changes
- Finish ordinary S57 row closure
- Finish the remaining internal/meta lookup-row compiler closure
- Do not remove ordinary S57 rows from coverage accounting

## Done when
- `compiler_missing_lookup_row` is gone from the committed inventory baseline
- ordinary S57 rows no longer depend on missing compiler-side assets
- the inventory baseline no longer reports unsupported rows

## Verification
- Build the relevant compiler/lookup/symbolizer targets
- Run focused inventory/compiler/lookup/symbolizer tests
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
