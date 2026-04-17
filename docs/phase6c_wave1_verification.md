# Phase 6C Wave 1 Verification

Phase 6C wave 1 closes the first compiler-first sweep over the pinned `Release_5.14.0/s57data` snapshot without widening the runtime ABI or changing host code.

## Achieved baseline

Wave-1 object families:

- `NOTMRK`
- `TERMNL`
- `BOYWTW`
- `BOYLAT`
- `TOPMAR`
- `BCNLAT`
- `HRBFAC`
- `POSITN`
- `OBSTRN`
- `RDOCAL`
- `VEHTRF`
- `RESARE`

Task-100 result:

- wave-1 rows no longer carry `compiler_missing_lookup_row`
- the committed inventory baseline remains:
  - `lookupRowsTotal = 3057`
  - `supportedRows = 8`
  - `partialRows = 2761`
  - `unsupportedRows = 288`

Task-101 result:

- compiler-side point-asset canonicalization cleared the formatting-only false gaps for:
  - `BOYWTW`
  - `RDOCAL`
  - `TOPMAR`
  - `VEHTRF`
- geometry-only point symbols such as `TOPMAR90` and `TOPMAR93` remain in the compiled path
- the remaining explicit wave-1 point-asset partials are still snapshot-backed:
  - `BOYLAT -> BOYSPH79`
  - `OBSTRN -> FLTHAZ02`
  - `RESARE -> ESSARE01`
  - `RESARE -> PSSARE01`

Task-102 result:

- repository-owned fixed-scene evidence now exists for:
  - `phase6c_wave1_topmarks_and_laterals_day_standard`
  - `phase6c_wave1_harbour_and_terminal_day_standard`
  - `phase6c_wave1_hazards_and_services_day_standard`
- `runtime.s52_resource_snapshot_inventory` now discovers all committed `*.reference.json` scene files, so `scene_harness_not_covered` reflects the full repository-owned harness surface rather than only the original Phase 6A scenes

Task-103 result:

- the retained three-chart real-world sample set now reports wave-1 family metrics:
  - `notices_and_terminals`
  - `lateral_and_waterway_marks`
  - `harbour_facilities_and_positions`
  - `hazards_and_services`
- observed retained-sample totals:
  - `notices_and_terminals{seen=0, s52Hits=0, preferredCompiledHits=0, fallbackHits=0, preferredTextInstructionHits=0}`
  - `lateral_and_waterway_marks{seen=232, s52Hits=232, preferredCompiledHits=232, fallbackHits=0, preferredTextInstructionHits=222}`
  - `harbour_facilities_and_positions{seen=0, s52Hits=0, preferredCompiledHits=0, fallbackHits=0, preferredTextInstructionHits=0}`
  - `hazards_and_services{seen=34, s52Hits=34, preferredCompiledHits=34, fallbackHits=0, preferredTextInstructionHits=4}`
- on the retained sample set:
  - `lateral_and_waterway_marks` and `hazards_and_services` are now proven by real-chart evidence with `preferredCompiledHits > 0` and `fallbackHits == 0`
  - `notices_and_terminals` and `harbour_facilities_and_positions` still rely on the task-102 fixed scenes because the retained sample set does not expose them

## Focused verification matrix

The focused task-104 matrix reran:

- `runtime.s52_resource_snapshot_inventory`
- `runtime.s52_catalog_compiler`
- `runtime.s52_lookup_model`
- `runtime.feature_symbolizer`
- `runtime.point_symbol`
- `runtime.feature_renderer`
- `runtime.chart1_s64_reference_harness`
- `runtime.s64_reference_smoke`
- `runtime.s57_lookup_coverage_smoke`
- `runtime.s57_real_chart_smoke`

Result:

- `10/10` passed

The retained real-chart smoke remained at:

- `C1511781`: `s52Hits=521`, `textCandidates=226`, `visibleLabels=43`
- `C1511782`: `s52Hits=1398`, `textCandidates=363`, `visibleLabels=72`
- `C1511783`: `s52Hits=378`, `textCandidates=188`, `visibleLabels=52`

## Remaining gaps

Wave 1 intentionally does **not** claim:

- full snapshot coverage across all non-wave-1 object families
- full repository-owned harness coverage across all `3057` lookup rows
- full S-64 pass
- full OpenCPN parity
- ECDIS compliance

The most important remaining gaps after wave 1 are:

- non-wave-1 object families that still sit outside this compiler-first sweep
- broader `scene_harness_not_covered` inventory rows outside the current committed fixed-scene set
- snapshot-backed point-asset absences such as `BOYSPH79`, `FLTHAZ02`, `ESSARE01`, and `PSSARE01`
- future waves that need to extend real-chart exposure for the currently fixed-scene-only wave-1 families
