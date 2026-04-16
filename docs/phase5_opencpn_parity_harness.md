# Phase 5 OpenCPN Parity Harness

This note defines the repository-owned parity harness for Phase 5 engineering cross-checks.

## Purpose

The parity harness exists to compare selected `chart_view` S57 behavior against OpenCPN-oriented reference samples for engineering purposes only.

It does **not** make OpenCPN the normative source, and it does **not** create a compliance claim.

Normative truth remains:

- IHO S-52
- Annex A
- S-64

## Harness shape

The repository now provides:

- a repeatable comparison script: [opencpn_parity_harness.ps1](/C:/Users/zsh/source/repos/chart_view/scripts/opencpn_parity_harness.ps1)
- committed sample files under [tests/data/parity](/C:/Users/zsh/source/repos/chart_view/tests/data/parity)

The harness compares a `chart_view` observation JSON against a curated engineering reference JSON.

The current JSON schema intentionally stays narrow:

- `sampleId`
- selected chart ids
- required chart ids
- optional required rule ids
- minimum expected counters such as:
  - `namedFeatures`
  - `unicodeNamed`
  - `textCandidates`
  - `visibleLabels`

## Current selected sample set

The first committed Phase 5 sample set uses the fixed S57 pair:

- `C1511781.000`
- `C1511782.000`

Files:

- reference sample: [phase5_s57_fixed_pair.reference.json](/C:/Users/zsh/source/repos/chart_view/tests/data/parity/phase5_s57_fixed_pair.reference.json)
- chart_view observation: [phase5_s57_fixed_pair.chart_view.json](/C:/Users/zsh/source/repos/chart_view/tests/data/parity/phase5_s57_fixed_pair.chart_view.json)

This keeps the harness aligned with the repository's already-verified real-pair smoke path.

## How to run

```powershell
powershell -ExecutionPolicy Bypass -File scripts/opencpn_parity_harness.ps1 `
  -Reference tests/data/parity/phase5_s57_fixed_pair.reference.json `
  -Observation tests/data/parity/phase5_s57_fixed_pair.chart_view.json
```

## How to use with external OpenCPN captures

When an engineer wants a real OpenCPN cross-check, the expected workflow is:

1. Open the chosen ENC sample in OpenCPN.
2. Record the selected cross-check facts into the same JSON shape.
3. Run the harness against the `chart_view` observation.
4. Treat differences as engineering investigation inputs, not as normative failures by themselves.

## Scope boundary

Task 78 adds the repeatable harness and the first committed reference sample format.

It does not yet:

- automate OpenCPN launch or screenshot capture
- claim parity for every S57 behavior
- replace the later broader real-chart smoke and final verification work
