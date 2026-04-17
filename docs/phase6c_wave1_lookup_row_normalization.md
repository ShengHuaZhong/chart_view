# Phase 6C Wave 1 Lookup-Row Normalization

Task 100 starts the Phase 6C compiler-first wave by correcting the inventory/compiler row-matching boundary for the selected wave-1 families:

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

The main change in this task is **not** new renderer behavior. The runtime lookup path already consumes normalized compiled rows. The task-100 fix normalizes the inventory-side row key so source rows from the pinned OpenCPN `chartsymbols.xml` snapshot match the compiled rows that already exist in the catalog.

This means task 100 intentionally addresses:

- false `compiler_missing_lookup_row` reports for wave-1 families
- deterministic inventory accounting for normalized object acronyms
- focused regression evidence that wave-1 rows now show up as compiled-covered, partial, or supported

This task does **not** yet canonicalize the remaining wave-1 point assets such as:

- `FLTHAZ02`
- `BOYLAT52/53/54/55/56`
- `BOYSPP50`
- `VEHTRF01`
- `RDOCAL02/03`
- `ESSARE01`
- `PSSARE01`

Those stay explicitly in task 101.
