# Phase 6A Chart 1 / S-64 Graphical Reference Harness

This note records the repository-owned graphical reference harness added in `91a-chart1-s64-graphical-reference-harness`.

## Purpose

The harness provides a fixed-scene graphical regression line for:

- one Chart 1-inspired harbor scene
- two selected S-64-inspired point/text scenes

It extends the earlier counter-based smokes by adding:

- committed crop hashes
- committed object-level rule/style assertions
- committed label visibility counts

Normative truth remains:

- IHO S-52
- Annex A
- S-64

OpenCPN is not used here as a pass/fail oracle. That engineering delta work remains in `92a`.

## Test target

- target: [chart1_s64_reference_harness_tests.cpp](/C:/Users/zsh/source/repos/chart_view/test/runtime/chart1_s64_reference_harness_tests.cpp)
- ctest name: `runtime.chart1_s64_reference_harness`

The target renders fixed synthetic S57 datasets through the runtime-owned Phase 6A portrayal path and compares the current observation against committed JSON references under [tests/data/reference](/C:/Users/zsh/source/repos/chart_view/tests/data/reference).

## Fixed scenes

Committed reference files:

- [phase6a_chart1_day_standard.reference.json](/C:/Users/zsh/source/repos/chart_view/tests/data/reference/phase6a_chart1_day_standard.reference.json)
- [phase6a_s64_traditional.reference.json](/C:/Users/zsh/source/repos/chart_view/tests/data/reference/phase6a_s64_traditional.reference.json)
- [phase6a_s64_simplified.reference.json](/C:/Users/zsh/source/repos/chart_view/tests/data/reference/phase6a_s64_simplified.reference.json)

Scene coverage:

- `phase6a_chart1_day_standard`
  - `LNDARE`, `FAIRWY`, `BOYSPP`, `WRECKS`
  - day palette
  - standard display category
  - traditional point symbols
- `phase6a_s64_traditional`
  - `BOYSPP`, `SOUNDG`, `WRECKS`
  - day palette
  - traditional point symbols
  - soundings and text enabled
- `phase6a_s64_simplified`
  - `BOYSPP`, `SOUNDG`, `WRECKS`
  - day palette
  - simplified point symbols
  - soundings and text suppressed

## Observation schema

Each committed reference records:

- scene id and display-mode summary
- object-level observations:
  - `featureId`
  - `objectAcronym`
  - `ruleId`
  - `styleKey`
  - `textAttributeKey`
  - `primaryAssetId`
  - `suppressed`
- crop observations:
  - crop name
  - crop rectangle
  - 64-bit FNV-1a hash of RGBA crop bytes
  - non-background pixel count
- `visibleLabelCount`
- `unicodeVisibleLabelCount`

The harness keeps the JSON deliberately narrow so the committed references remain reviewable.

## Regenerating references

When an intentional portrayal change lands, regenerate the committed references with:

```powershell
$env:CHART_VIEW_WRITE_REFERENCE = '1'
ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R '^runtime\.chart1_s64_reference_harness$' --output-on-failure
Remove-Item Env:CHART_VIEW_WRITE_REFERENCE
```

This updates the files in [tests/data/reference](/C:/Users/zsh/source/repos/chart_view/tests/data/reference).

## Scope boundary

Task `91a` adds the normative fixed-scene reference harness only.

It does not yet:

- compare against OpenCPN captures
- claim full Chart 1 coverage
- claim full S-64 matrix coverage
- make compliance claims
