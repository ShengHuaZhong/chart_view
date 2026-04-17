# Phase 6C Wave 1 Real-Chart Family Metrics

Task 103 extends `runtime.s57_lookup_coverage_smoke` so the retained three-chart real-world sample set reports explicit wave-1 family metrics without widening the runtime ABI or changing host code.

The wave-1 buckets are:

- `notices_and_terminals`
  - `NOTMRK`
  - `TERMNL`
- `lateral_and_waterway_marks`
  - `BOYLAT`
  - `BOYWTW`
  - `BCNLAT`
  - `TOPMAR`
- `harbour_facilities_and_positions`
  - `HRBFAC`
  - `POSITN`
- `hazards_and_services`
  - `OBSTRN`
  - `RDOCAL`
  - `VEHTRF`
  - `RESARE`

For each bucket the smoke test now reports:

- `seen`
- `s52Hits`
- `preferredCompiledHits`
- `fallbackHits`
- `preferredTextInstructionHits`

Observed task-103 result on the retained real-chart sample set:

- `C1511781`
  - `lateral_and_waterway_marks{seen=24, s52Hits=24, preferredCompiledHits=24, fallbackHits=0, preferredTextInstructionHits=22}`
  - `hazards_and_services{seen=25, s52Hits=25, preferredCompiledHits=25, fallbackHits=0, preferredTextInstructionHits=4}`
- `C1511782`
  - `lateral_and_waterway_marks{seen=135, s52Hits=135, preferredCompiledHits=135, fallbackHits=0, preferredTextInstructionHits=127}`
  - `hazards_and_services{seen=4, s52Hits=4, preferredCompiledHits=4, fallbackHits=0, preferredTextInstructionHits=0}`
- `C1511783`
  - `lateral_and_waterway_marks{seen=73, s52Hits=73, preferredCompiledHits=73, fallbackHits=0, preferredTextInstructionHits=73}`
  - `hazards_and_services{seen=5, s52Hits=5, preferredCompiledHits=5, fallbackHits=0, preferredTextInstructionHits=0}`

Wave-1 totals on the retained sample set:

- `notices_and_terminals{seen=0, s52Hits=0, preferredCompiledHits=0, fallbackHits=0, preferredTextInstructionHits=0}`
- `lateral_and_waterway_marks{seen=232, s52Hits=232, preferredCompiledHits=232, fallbackHits=0, preferredTextInstructionHits=222}`
- `harbour_facilities_and_positions{seen=0, s52Hits=0, preferredCompiledHits=0, fallbackHits=0, preferredTextInstructionHits=0}`
- `hazards_and_services{seen=34, s52Hits=34, preferredCompiledHits=34, fallbackHits=0, preferredTextInstructionHits=4}`

Task-103 interpretation:

- the retained three-chart sample set already exercises:
  - lateral and waterway marks
  - hazard and service families
- the retained three-chart sample set does **not** currently expose:
  - notices and terminals
  - harbour facilities and positions

Those two zero-exposure families remain covered by the committed task-102 fixed scenes only. Task 103 intentionally does not add new chart-selection logic or widen the real-chart sample set.
