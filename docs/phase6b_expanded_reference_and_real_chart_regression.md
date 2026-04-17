# Phase 6B Expanded Reference And Real-Chart Regression

## Scope

Task `98-s52-expanded-reference-and-real-chart-regression` expands the repository-owned
graphical evidence around the existing `chartsymbols.xml -> compiled IR -> runtime renderer`
pipeline without widening the runtime ABI or moving logic into the host layers.

The task keeps OpenCPN as an engineering input and delta reference only. The primary
acceptance evidence remains:

- repository-owned fixed-scene graphical references
- broader real-chart S57 smoke
- real-chart lookup coverage metrics for the chosen inventory-covered families

## Fixed-Scene Reference Set

The `runtime.chart1_s64_reference_harness` target now covers eight committed scenes:

- `phase6a_chart1_day_standard`
- `phase6a_s64_traditional`
- `phase6a_s64_simplified`
- `phase6b_aids_to_navigation_day_standard`
- `phase6b_lights_hazards_day_standard`
- `phase6b_line_family_day_standard`
- `phase6b_area_family_symbolized`
- `phase6b_text_name_selection_day_standard`

The new Phase 6B scenes are organized by symbol family and capture:

- point aids to navigation (`BOYSPP`, `BCNSPP`)
- lights and hazard points (`LIGHTS`, `OBSTRN`, `WRECKS`)
- line families (`COALNE`, `DEPCNT`, `FAIRWY`, `PIPSOL`)
- area families (`LNDARE`, `DEPARE`, `ACHARE`, `RESARE`)
- text-heavy name selection (`OBJNAM` / `NOBJNM`)

Each committed reference JSON preserves:

- object-level `ruleId`, `sourceRcid`, `tableName`, `styleKey`
- `textAttributeKey`
- `primaryAssetId`
- `conditionIds`
- crop hashes and non-background pixel counts
- scene-level visible / Unicode-visible label counts

## Real-Chart Lookup Coverage Metrics

`runtime.s57_lookup_coverage_smoke` now reports family metrics for the fixed pair plus one
additional readable S57 chart:

- selected charts:
  - `C1511781`
  - `C1511782`
  - `C1511783`
- total lookup metrics:
  - `totalS52Hits = 2297`
  - `totalPreferredCompiledHits = 2297`
  - `totalFallbackCompiledHits = 0`
  - `totalPreferredTextInstructionHits = 877`

Phase 6B family totals on that sample set:

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

These metrics are intended as repository progress evidence, not as a compliance claim.

## Broader Real-Chart Smoke

`runtime.s57_real_chart_smoke` remains part of the acceptance surface and currently passes on:

- `C1511781`
- `C1511782`
- `C1511783`

The current real-chart evidence from the task-98 rerun is:

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

## Scope Boundaries

This task does not:

- change the runtime public ABI
- change `chart_qtwidgets` or `chart_standalone`
- widen the OpenCPN role beyond engineering input/delta reference
- claim full S-64 pass, full OpenCPN parity, or ECDIS compliance
