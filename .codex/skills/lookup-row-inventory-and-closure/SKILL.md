# lookup-row-inventory-and-closure

## Purpose

Audit, classify, and close S-52 lookup-row backlog with explicit cause buckets and honest ordinary-vs-meta accounting.

This skill is for Phase 6D inventory, backlog decomposition, and closure tracking.

## Use this skill when

- Auditing remaining lookup rows
- Recomputing backlog counts
- Assigning rows to task 106 / 107 / 108 / 109 / 110
- Checking whether a task actually reduced the intended bucket
- Preparing final completion evidence

## Read first

1. `AGENTS.md`
2. `plan.md`
3. Current Phase 6D task file
4. Current committed inventory baseline
5. Any generated inventory JSON/CSV
6. Existing audit scripts
7. `state/current_iteration.md`

## Mandatory accounting rules

Always separate:

- ordinary S57 rows
- internal/meta rows

Never use internal/meta accounting to hide ordinary backlog.

Always report at least:

- `lookupRowsTotal`
- `supportedRows`
- `partialRows`
- `unsupportedRows`
- `ordinaryRows`
- `internalOrMetaRows`

## Cause-bucket taxonomy

Use these buckets unless there is strong evidence to add a justified new one:

- `snapshot_asset_available_but_not_reached`
- `supplemental_upstream_candidate`
- `manual_overlay_residual`
- `parser_or_compiler_spillover`
- `harness_only_partial`
- `internal_or_meta_row_only`
- `inventory_false_positive_or_misclassified`

Every non-fully-complete row must land in one explicit bucket.

## Bucket-to-task mapping defaults

- `supplemental_upstream_candidate` -> task 106
- `manual_overlay_residual` -> task 107
- `parser_or_compiler_spillover` -> task 108
- `internal_or_meta_row_only` -> task 108
- `harness_only_partial` -> task 109
- `inventory_false_positive_or_misclassified` -> task 110

## Required outputs

When used for auditing, produce:

- a human-readable audit doc
- a machine-readable row inventory
- per-bucket counts and percentages
- representative examples per bucket
- a statement on whether the current task chain is sufficient

## Closure rules

A row is not “fully complete” just because:
- an asset exists somewhere
- a fallback glyph appears
- a counter-based smoke passes
- a harness sees a non-zero hit

It is only fully complete if it satisfies the repository’s current completion definition for that row.

## Anti-patterns

Do not:
- keep using stale totals without recomputing
- silently move rows between ordinary and meta buckets
- declare 100% completion while a large `harness_only_partial` bucket remains
- let task 110 hide unresolved bucket logic