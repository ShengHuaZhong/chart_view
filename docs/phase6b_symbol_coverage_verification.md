# Phase 6B Symbol Coverage Verification

## Scope

Task `99-phase6b-symbol-coverage-verification` closes the Phase 6B symbol-coverage
expansion for the pinned OpenCPN resource snapshot:

- `vendor/opencpn_s57data/Release_5.14.0/s57data`

The task does not add new runtime or host features. It records the resulting coverage
baseline, the focused verification matrix for tasks `94-98`, and the remaining known
gaps after the resource-snapshot, parser/compiler, lookup/CSP, renderer, and expanded
reference work.

OpenCPN remains an engineering resource-format input and delta-harness reference only.
Normative portrayal truth remains IHO `S-52 / Annex A / S-64`.

## Achieved Phase 6B Baseline

Phase 6B now has a repository-owned, repeatable coverage baseline for the pinned
`chartsymbols.xml` snapshot and a verification surface that proves the chosen symbol
families run through the compiled runtime path instead of the older built-in/private
fallback path.

### Resource-snapshot baseline

The committed inventory baseline at:

- `tests/data/reference/phase6b_s52_resource_snapshot_inventory.reference.json`

currently reports:

- `lookupRowsTotal = 3057`
- `supportedRows = 8`
- `partialRows = 1844`
- `unsupportedRows = 1205`
- `supportedInstructionTokens = 8`
- `unsupportedInstructionTokens = 0`
- `supportedConditionalTokens = 22`
- `unsupportedConditionalTokens = 0`

Current dominant degraded-row reasons remain:

- `scene_harness_not_covered = 3049`
- `compiler_missing_lookup_row = 1205`
- `compiler_missing_point_asset:FLTHAZ02 = 16`
- `compiler_missing_point_asset:BOYLAT55 = 16`
- `compiler_missing_point_asset:BOYLAT56 = 14`

This means the major remaining Phase 6B work is now explicit and measurable rather than
hidden behind generic fallback behavior.

### Covered families through the compiled runtime path

The current covered-family baseline proven by the fixed-scene and real-chart evidence is:

- point families:
  - aids to navigation
  - hazard points
- line families:
  - coastline / contour / fairway / pipeline and other line-and-boundary paths
- area families:
  - land / depth / anchorage / restricted-area patterns
- text families:
  - instruction-selected `OBJNAM` / `NOBJNM` paths on the covered scenes
- conditional families:
  - the Phase 6B family sweep condition ids added through task `96`

### Expanded repository-owned graphical references

The committed fixed-scene harness now replays eight scenes:

- `phase6a_chart1_day_standard`
- `phase6a_s64_traditional`
- `phase6a_s64_simplified`
- `phase6b_aids_to_navigation_day_standard`
- `phase6b_lights_hazards_day_standard`
- `phase6b_line_family_day_standard`
- `phase6b_area_family_symbolized`
- `phase6b_text_name_selection_day_standard`

These scenes preserve:

- object-level `ruleId`, `sourceRcid`, `tableName`, `styleKey`
- `primaryAssetId`
- `textAttributeKey`
- `conditionIds`
- crop hashes and non-background pixel counts
- scene-level visible / Unicode-visible label counts

## Real-Chart Evidence

`runtime.s57_lookup_coverage_smoke` currently reports these totals on the selected
real-chart sample set `C1511781`, `C1511782`, and `C1511783`:

- `totalS52Hits = 2297`
- `totalPreferredCompiledHits = 2297`
- `totalFallbackCompiledHits = 0`
- `totalPreferredTextInstructionHits = 877`

Family-level totals on that sample set:

- `aid_to_navigation`
  - `seen = 490`
  - `preferredCompiledHits = 490`
  - `fallbackHits = 0`
  - `preferredTextInstructionHits = 220`
- `hazard_points`
  - `seen = 31`
  - `preferredCompiledHits = 31`
  - `fallbackHits = 0`
- `line_and_boundary`
  - `seen = 420`
  - `preferredCompiledHits = 420`
  - `fallbackHits = 0`
  - `preferredTextInstructionHits = 2`
- `area_patterns`
  - `seen = 404`
  - `preferredCompiledHits = 404`
  - `fallbackHits = 0`
  - `preferredTextInstructionHits = 47`
- `named_text`
  - `seen = 289`
  - `preferredCompiledHits = 288`
  - `fallbackHits = 0`
  - `preferredTextInstructionHits = 288`

`runtime.s57_real_chart_smoke` currently passes on:

- `C1511781`
  - `s52Hits = 521`
  - `named = 86`
  - `unicodeNamed = 48`
  - `textCandidates = 226`
  - `visibleLabels = 43`
  - `visibleUnicodeLabels = 8`
- `C1511782`
  - `s52Hits = 1398`
  - `named = 134`
  - `unicodeNamed = 21`
  - `textCandidates = 363`
  - `visibleLabels = 72`
  - `visibleUnicodeLabels = 6`
- `C1511783`
  - `s52Hits = 378`
  - `named = 69`
  - `unicodeNamed = 6`
  - `textCandidates = 188`
  - `visibleLabels = 52`
  - `visibleUnicodeLabels = 1`

## Engineering Delta Status

The retained OpenCPN engineering delta harness still covers the curated Phase 6A
three-scene subset and currently reports:

- `totalScenes = 3`
- `totalComparableFeatures = 9`
- `totalFeatureDeltas = 0`
- `totalCropDeltas = 0`

This remains engineering evidence only. The primary Phase 6B acceptance line is the
repository-owned fixed-scene harness plus the broader real-chart smoke/coverage tests.

## Remaining Gaps

Phase 6B closes the current symbol-coverage expansion baseline, but it does not yet
provide:

- full lookup-row coverage for the vendored `chartsymbols.xml` snapshot
- full compiler normalization for the remaining `1205` unsupported lookup rows
- full fixed-scene coverage for the `3049` rows still tagged `scene_harness_not_covered`
- full point-asset coverage for the remaining unsupported point-asset families such as
  `FLTHAZ02` and `BOYLAT5x`
- full S-64 matrix pass
- full OpenCPN parity on all possible scenes
- ECDIS compliance

`FAIRWY` scene-model mismatch remains an informational reference-note issue unless a
later task explicitly unifies the inventory and fixed-scene geometry model for that
family.

## Final Focused Verification Matrix

Task `99` closes Phase 6B against this focused matrix:

- inventory / parser / compiler
  - `runtime.s52_resource_snapshot_inventory`
  - `assets.opencpn_resource_bundle`
  - `runtime.opencpn_chartsymbols_parser`
  - `runtime.s52_instruction_string_parser`
  - `runtime.s52_catalog_compiler`
- lookup / CSP / symbolization
  - `runtime.s52_lookup_model`
  - `runtime.s52_conditional_symbology`
  - `runtime.feature_symbolizer`
- renderer
  - `runtime.feature_renderer`
  - `runtime.point_symbol`
  - `runtime.line_symbol`
  - `runtime.area_symbol`
  - `runtime.label`
- reference and real-chart evidence
  - `runtime.chart1_s64_reference_harness`
  - `runtime.s64_reference_smoke`
  - `runtime.s57_lookup_coverage_smoke`
  - `runtime.s57_real_chart_smoke`
- engineering delta
  - `runtime.opencpn_visual_delta_harness`

This task records the current Phase 6B baseline only. It does not claim compliance.
