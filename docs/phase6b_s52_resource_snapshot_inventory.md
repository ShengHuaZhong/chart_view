# Phase 6B S-52 Resource Snapshot Coverage Inventory

## Purpose

Task 94 establishes a deterministic, repo-owned coverage baseline for the pinned OpenCPN
`Release_5.14.0/s57data/chartsymbols.xml` snapshot before any Phase 6B parser/compiler/lookup/renderer expansions.

The baseline inventory lives at:

- `tests/data/reference/phase6b_s52_resource_snapshot_inventory.reference.json`

It is generated from:

- `vendor/opencpn_s57data/Release_5.14.0/s57data/chartsymbols.xml`
- the current `OpenCpnChartsymbolsParser`
- the current `S52InstructionStringParser`
- the current `S52SourceCatalogCompiler`
- the current fixed-scene harness references:
  - `phase6a_chart1_day_standard`
  - `phase6a_s64_traditional`
  - `phase6a_s64_simplified`

## Baseline summary

At the task-94 baseline the committed inventory reports:

- `lookupRowsTotal = 3057`
- `supportedRows = 6`
- `partialRows = 1839`
- `unsupportedRows = 1212`
- `coveredRuleIds = 7`
- `coveredSourceRcids = 7`

Current parser/compiler token status summary:

- supported instruction tokens: `7`
- partial instruction tokens: `0`
- unsupported instruction tokens: `1`
- supported conditional tokens: `16`
- unsupported conditional tokens: `6`

Current top degraded-row reasons are:

- `scene_harness_not_covered = 3049`
- `compiler_missing_lookup_row = 1205`
- `parser_unsupported:AC = 244`

This means the main remaining Phase 6B work is not hidden anymore:

1. parser/compiler coverage still drops `AC(...)`
2. a large set of rows still does not survive source-catalog -> compiled-row normalization
3. current fixed-scene references only exercise a very small subset of the snapshot
4. several synthetic line-style asset references still do not resolve to compiled assets

## Regeneration

The committed reference is updated by the focused inventory test:

```powershell
$env:CHART_VIEW_WRITE_REFERENCE='1'
ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R '^runtime\.s52_resource_snapshot_inventory$' --output-on-failure
Remove-Item Env:CHART_VIEW_WRITE_REFERENCE
```

Normal verification uses the same test without the environment variable and compares the generated document against the committed baseline.
