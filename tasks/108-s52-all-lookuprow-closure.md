# 108 - Phase 6D workstream A: IR compiler normalization and alias closure

## Objective
Move the pinned fallback input closer to `chart_view`'s own canonical IR by
closing compiler-owned normalization, alias, and spillover gaps without widening
the runtime public ABI or touching the host architecture.

## Phase
- Phase 6D

## Layer
- runtime
- portrayal
- tests
- docs

## Depends on
- 107
- 107a

## In scope
- compiler-owned normalization and aliasing for currently explicit fallback-path
  legacy or spillover tokens
- machine-readable input to canonical-IR boundary cleanup
- raw-instruction preservation while compiled instructions move to canonical
  internal token names
- focused inventory/compiler/lookup regressions
- explicit documentation of the normalization boundary

## Out of scope
- No CSP behavior expansion
- No new manual overlays beyond task `107` / task `107a`
- No host changes
- No public ABI changes
- No coverage-accounting tricks

## Required changes
- Audit and normalize the currently explicit legacy / spillover token forms that
  the fallback compiler path still emits or consumes.
- Establish a compiler-owned alias / normalization layer for those explicit
  tokens so runtime execution no longer treats raw legacy naming as final truth.
- Preserve the approved resource order:
  1. pinned base snapshot
  2. supplemental OpenCPN resources
  3. repo-owned manual overlay
- Keep ordinary S57 backlog explicit; do not hide unresolved inland / residual /
  internal-meta buckets by changing accounting.

## Done when
- the current workstream's explicit legacy / spillover token set is normalized
  into canonical IR tokens during compilation
- ordinary rows no longer remain partial only because of aliasable compiler-side
  naming noise on the covered token set
- the repo records the machine-readable input versus canonical IR truth boundary
- no public ABI or host architecture changes were required

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
