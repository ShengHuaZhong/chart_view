# Phase 6C Wave 1 Reference Harness

Task 102 adds repository-owned graphical evidence for the Phase 6C wave-1 object families without widening the runtime ABI or changing host code.

The wave-1 harness extends `runtime.chart1_s64_reference_harness` with three committed day/standard scenes:

- `phase6c_wave1_topmarks_and_laterals_day_standard`
  - covers `BOYLAT`, `BOYWTW`, `BCNLAT`, and `TOPMAR`
  - committed crops:
    - `lateral_buoy_symbol`
    - `waterway_buoy_symbol`
    - `beacon_topmark_symbol`
- `phase6c_wave1_harbour_and_terminal_day_standard`
  - covers `NOTMRK`, `TERMNL`, `HRBFAC`, and `POSITN`
  - committed crops:
    - `notice_mark_symbol`
    - `terminal_symbol`
    - `harbour_facility_symbol`
- `phase6c_wave1_hazards_and_services_day_standard`
  - covers `OBSTRN`, `RDOCAL`, `VEHTRF`, and `RESARE`
  - committed crops:
    - `obstruction_symbol`
    - `radio_call_symbol`
    - `traffic_symbol`
    - `restricted_area_symbol`

Each committed reference keeps object-level assertions for:

- `ruleId`
- `sourceRcid`
- `tableName`
- `primaryAssetId`
- `textAttributeKey`
- `conditionIds`

Task 102 also broadens the inventory harness boundary so `runtime.s52_resource_snapshot_inventory` now discovers all committed `*.reference.json` files under `tests/data/reference`, excluding only the inventory baseline itself. That keeps `scene_harness_not_covered` tied to the full repository-owned reference set instead of the original Phase 6A subset only.

Observed task-102 result on the refreshed inventory baseline:

- the three new wave-1 scenes replay cleanly from committed reference JSON in non-write mode
- the wave-1 object families now have repository-owned graphical evidence beyond the original Phase 6A and Phase 6B scenes
- the remaining wave-1 degraded rows are still dominated by:
  - `scene_harness_not_covered` for uncovered rows within the broader family
  - snapshot-backed point-asset gaps such as `BOYSPH79`, `FLTHAZ02`, `ESSARE01`, and `PSSARE01`

Task 102 intentionally does **not** add real-chart family metrics or broaden the runtime API. Those remain task 103 work.
