# Phase 6A OpenCPN Visual Delta Harness

This note records the engineering-only OpenCPN delta harness added in `92a-opencpn-visual-delta-harness`.

## Purpose

The harness compares the repository-owned fixed-scene observations from `91a` against curated expectations derived from the vendored OpenCPN `chartsymbols.xml` snapshot.

It exists to answer:

- which `RCID / table-name / asset / conditional` facts currently align with the vendored OpenCPN resource bundle
- which fixed-scene crops are visibly present or absent relative to those expectations
- where the remaining differences most likely sit: lookup selection, point-symbol mode, text/conditional handling, or geometry-shape mismatch

It does **not** make OpenCPN the normative oracle.

Normative truth remains:

- IHO S-52
- Annex A
- S-64

## Inputs

Engineering source snapshot:

- [vendor/opencpn_s57data/Release_5.14.0/s57data/chartsymbols.xml](/C:/Users/zsh/source/repos/chart_view/vendor/opencpn_s57data/Release_5.14.0/s57data/chartsymbols.xml)

Committed repository-owned observations from `91a`:

- [phase6a_chart1_day_standard.reference.json](/C:/Users/zsh/source/repos/chart_view/tests/data/reference/phase6a_chart1_day_standard.reference.json)
- [phase6a_s64_traditional.reference.json](/C:/Users/zsh/source/repos/chart_view/tests/data/reference/phase6a_s64_traditional.reference.json)
- [phase6a_s64_simplified.reference.json](/C:/Users/zsh/source/repos/chart_view/tests/data/reference/phase6a_s64_simplified.reference.json)

Curated OpenCPN comparison manifests:

- [phase6a_chart1_day_standard.opencpn.json](/C:/Users/zsh/source/repos/chart_view/tests/data/opencpn/phase6a_chart1_day_standard.opencpn.json)
- [phase6a_s64_traditional.opencpn.json](/C:/Users/zsh/source/repos/chart_view/tests/data/opencpn/phase6a_s64_traditional.opencpn.json)
- [phase6a_s64_simplified.opencpn.json](/C:/Users/zsh/source/repos/chart_view/tests/data/opencpn/phase6a_s64_simplified.opencpn.json)

## Harness shape

The harness is a script:

- [opencpn_visual_delta_harness.ps1](/C:/Users/zsh/source/repos/chart_view/scripts/opencpn_visual_delta_harness.ps1)

It loads:

- all `*.opencpn.json` manifests under `tests/data/opencpn`
- the matching `*.reference.json` observations under `tests/data/reference`

For each fixed scene it writes an engineering delta report under the chosen output directory.

The report includes:

- scene id and source snapshot
- per-feature comparisons for:
  - `objectAcronym`
  - `sourceRcid`
  - `tableName`
  - `primaryAssetId`
  - `textAttributeKey`
  - `conditionIds`
  - `suppressed`
- per-crop comparisons for:
  - crop presence
  - current crop hash
  - non-background pixel count
  - the expected OpenCPN visual token or note
- observed rule / table / RCID statistics for the scene

## Why the manifests are curated

The repository does not directly embed `s52plib`, and this task does not automate OpenCPN screenshot capture.

Instead, the manifests capture the `chartsymbols.xml` facts that are stable enough for engineering deltas:

- selected `RCID`
- selected `table-name`
- selected asset or conditional identifiers
- scene-specific notes about why a feature is comparable or only informational

This keeps the harness narrow and reviewable while still surfacing where `chart_view` diverges from the vendored OpenCPN resource model.

## Current fixed-scene comparison coverage

- `phase6a_chart1_day_standard`
  - strict comparisons for `LNDARE`, `BOYSPP`, and `WRECKS`
  - informational handling for `FAIRWY`, because the fixed repository scene uses line geometry while the OpenCPN snapshot exposes area-oriented FAIRWY lookups
- `phase6a_s64_traditional`
  - strict comparisons for traditional-mode `BOYSPP`, `SOUNDG`, and `WRECKS`
- `phase6a_s64_simplified`
  - strict comparisons for simplified-mode `BOYSPP`, `SOUNDG`, and `WRECKS`

## How to run

```powershell
powershell -ExecutionPolicy Bypass -NoProfile -File scripts/opencpn_visual_delta_harness.ps1 `
  -ManifestDir tests/data/opencpn `
  -ObservationDir tests/data/reference `
  -OutDir out/build/windows-msvc-debug/opencpn_visual_delta
```

The script exits non-zero only on structural problems:

- missing manifests
- missing observations
- duplicate scene ids / feature ids / crop names
- a manifest with zero comparable features

The script does **not** fail just because deltas exist. Those deltas are the point of the harness.

## Scope boundary

Task `92a` adds:

- scene-aligned engineering manifests
- deterministic delta-report generation
- a repeatable CTest entry for the harness

It does not:

- change normative pass/fail away from IHO references
- automate OpenCPN launch
- claim visual parity
- replace the final Phase 6A verification note in `93a`
