# s52-asset-pipeline

## Purpose

Handle S-52 resource ingest, provenance, fallback precedence, and compiler-facing asset packaging without widening runtime ABI or mixing phases.

This skill is the default for Phase 6D asset work.

## Use this skill when

- Working on tasks 106 or 107
- Vendoring supplemental OpenCPN resource files
- Creating repo-owned manual overlays
- Adjusting asset resolution precedence
- Updating compiler-facing asset manifests or provenance docs

## Read first

1. `AGENTS.md`
2. `plan.md`
3. The current task file
4. Any Phase 6D lookup-row completion docs
5. Existing vendored `s57data` snapshot layout
6. Existing compiler/loader code
7. Existing resource inventory / manifests

## Normative and engineering source rules

- Normative truth remains:
  - IHO S-52
  - Annex A
  - S-64
- Approved engineering fallback input path:
  1. pinned vendored OpenCPN snapshot
  2. GPL-compatible supplemental OpenCPN resource files
  3. repo-owned manual overlays for true residuals only

Never reverse this order.

## Asset resolution order

Always preserve:

1. base pinned snapshot
2. supplemental upstream resource pack
3. manual overlay pack

Do not create manual overlays for IDs that can honestly be satisfied by approved upstream supplemental resources.

## Provenance requirements

Every new vendored or overlay asset must have traceable metadata:

- asset id
- source category (`base`, `supplemental`, `manual_overlay`)
- exact upstream source note
- reason for inclusion
- reason manual creation was necessary, if applicable
- task that introduced it

## Required outputs

When this skill is used, expect to produce some combination of:

- vendored resource files
- manifest updates
- provenance notes
- compiler/loader precedence changes
- targeted verification docs
- reduced missing-asset inventory

## Verification expectations

You must be able to answer:

- which target IDs were absent in the base snapshot
- which were found in supplemental upstream resources
- which still remain residual for manual overlay
- whether loader/compiler precedence now resolves them in the correct order

## Anti-patterns

Do not:
- copy OpenCPN code
- treat OpenCPN resources as normative truth
- bypass provenance
- “fix” parser/compiler spillover IDs by drawing fake symbols first
- let manual overlays silently outrank real upstream resources