# Phase 6D Remaining Lookup-Row Cause-Bucket Audit

## Scope

This note is a **Phase 6D preflight audit** only. It does **not** complete
tasks `106-110`, does **not** change runtime/host/ABI behavior, and does **not**
advance task state. Its job is to answer one question with reproducible evidence:

> What is the current committed lookup-row backlog, and which cause bucket does
> each remaining non-fully-complete row belong to?

## Current committed baseline

The current committed inventory baseline comes from:

- `tests/data/reference/phase6b_s52_resource_snapshot_inventory.reference.json`

The matching reproducibility inputs are:

- pinned snapshot:
  `vendor/opencpn_s57data/Release_5.14.0/s57data/chartsymbols.xml`
- inventory generator/test already in the repo:
  - `test/support/s52_resource_snapshot_inventory.cpp`
  - `test/runtime/s52_resource_snapshot_inventory_tests.cpp`

This audit reclassifies the **currently committed** degraded rows from that
baseline. It does **not** trust the older `3049` number.

## Baseline answer: `3049` is stale

The current committed baseline is:

- `lookupRowsTotal = 3057`
- `supportedRows = 27`
- `partialRows = 2742`
- `unsupportedRows = 288`
- `degradedRows = 3030`

So the current real backlog for “not fully complete” rows is:

- **overall backlog = `3030`**

The older `3049` number is stale because it predates the current committed
baseline after the Phase 6C wave-1 normalization work. The current reference
JSON and `runtime.s52_resource_snapshot_inventory` target both resolve to
`3030`, not `3049`.

## Ordinary S57 vs internal/meta split

This audit splits rows into:

- **ordinary S57 rows**
- **internal/meta rows**

Internal/meta rows are only:

- object acronym starts with `$`
- or object acronym equals `######`

Current split:

| Slice | Rows |
| --- | ---: |
| Ordinary S57 degraded rows | 2742 |
| Internal/meta degraded rows | 288 |
| Total degraded rows | 3030 |

Important observation:

- all current **ordinary** backlog rows are `partial`
- all current **internal/meta** backlog rows are `unsupported`

So the ordinary backlog is no longer hidden inside `unsupported`; it sits almost
entirely in `partial`, and almost all of it is harness-related.

## Cause buckets

This audit used the following bucket taxonomy:

| Bucket | Meaning |
| --- | --- |
| `snapshot_asset_available_but_not_reached` | Asset is defined in the pinned snapshot, but compiler/loader/precedence does not reach it. |
| `supplemental_upstream_candidate` | Pinned snapshot does not define the asset, but the row looks like a normal missing-asset gap that task `106` should try to source from GPL-compatible supplemental OpenCPN resources. |
| `manual_overlay_residual` | Snapshot does not define the asset and the Phase 6D plan already treats it as a likely manual overlay residual for task `107`. |
| `parser_or_compiler_spillover` | Malformed/combined asset-text noise or compiler spillover; should be cleaned in task `108`, not solved by drawing symbols first. |
| `harness_only_partial` | Logic and assets appear present enough to compile, but the only remaining reason is missing repository-owned graphical/reference evidence. |
| `internal_or_meta_row_only` | Internal/meta rows, tracked separately from ordinary S57 rows. |
| `inventory_false_positive_or_misclassified` | Could not be confidently assigned elsewhere and needs explicit audit review. |

## Cause-bucket breakdown

### Overall breakdown

| Bucket | Total | Share of backlog | Ordinary S57 | Internal/meta |
| --- | ---: | ---: | ---: | ---: |
| `harness_only_partial` | 2650 | 87.46% | 2650 | 0 |
| `internal_or_meta_row_only` | 288 | 9.50% | 0 | 288 |
| `supplemental_upstream_candidate` | 54 | 1.78% | 54 | 0 |
| `manual_overlay_residual` | 21 | 0.69% | 21 | 0 |
| `parser_or_compiler_spillover` | 17 | 0.56% | 17 | 0 |
| `snapshot_asset_available_but_not_reached` | 0 | 0.00% | 0 | 0 |
| `inventory_false_positive_or_misclassified` | 0 | 0.00% | 0 | 0 |

### Key interpretation

- The backlog is **not** mainly an overlay problem.
- The dominant bucket is **`harness_only_partial`**, not compiler closure.
- Under the current strict definition-node audit of the pinned snapshot:
  - **no rows** landed in `snapshot_asset_available_but_not_reached`
  - **no rows** landed in `inventory_false_positive_or_misclassified`

That means the current remaining non-harness asset gaps are either:

- plausible supplemental-upstream candidates for task `106`
- explicit manual-overlay residuals for task `107`
- or parser/compiler spillover for task `108`

## Representative rows by bucket

The full machine-readable list is in:

- `docs/generated/phase6d_lookuprow_cause_bucket_inventory.json`
- `docs/generated/phase6d_lookuprow_cause_bucket_inventory.csv`

The tables below show representative samples.

### `harness_only_partial` samples

Top families in this bucket:

- `NOTMRK=208`
- `DAYMAR=149`
- `BOYLAT=131`
- `TOPMAR=129`
- `LNDMRK=127`
- `BCNLAT=115`
- `TERMNL=113`
- `BOYSPP=73`
- `VEGATN=65`
- `BUISGL=56`

Representative rows:

| Row ID | Family | Representative asset/rule |
| --- | --- | --- |
| `ACHARE|Point|Paper|1712|30001` | `ACHARE` | `ACHARE02` |
| `achare|Point|Paper|2532|30603` | `ACHARE` | `ACHARE02` |
| `ACHARE|Point|Simplified|961|31013` | `ACHARE` | `ACHARE02` |
| `achare|Point|Simplified|1299|31351` | `ACHARE` | `ACHARE02` |
| `ACHARE|Area|Plain|1|32037` | `ACHARE` | `ACHARE02` |
| `ACHARE|Area|Plain|2|32038` | `ACHARE` | `ACHARE51` |
| `achare|Area|Plain|243|32278` | `ACHARE` | `ACHARE02` |
| `ACHARE|Area|Symbolized|342|32377` | `ACHARE` | `ACHARE51` |
| `ACHBRT|Point|Paper|1713|30002` | `ACHBRT` | `ACHARE03` |
| `ACHBRT|Point|Simplified|962|31014` | `ACHBRT` | `ACHARE03` |

### `internal_or_meta_row_only` samples

Top families in this bucket:

- `$CSYMB=178`
- `$AREAS=54`
- `$LINES=46`
- `######=5`
- `$TEXTS=5`

Representative rows:

| Row ID | Family | Representative asset/rule |
| --- | --- | --- |
| `######|Point|Paper|1711|30000` | `######` | `QUESMRK1` |
| `######|Point|Simplified|960|31012` | `######` | `QUESMRK1` |
| `######|Line|Lines|687|31763` | `######` | `QUESMRK1` |
| `######|Area|Plain|0|32036` | `######` | `LS_DASH_1_CHMGD` |
| `######|Area|Symbolized|340|32375` | `######` | `LS_DASH_1_CHMGD` |
| `$AREAS|Area|Plain|312|32347` | `$AREAS` | `LS_SOLD_2_CHGRD` |
| `$AREAS|Area|Plain|313|32348` | `$AREAS` | `CHCRDEL1` |
| `$AREAS|Area|Plain|314|32349` | `$AREAS` | `CHCRID01` |
| `$AREAS|Area|Plain|315|32350` | `$AREAS` | `DIAMOND1` |
| `$AREAS|Area|Plain|316|32351` | `$AREAS` | `OVERSC01` |

### `supplemental_upstream_candidate` samples

Top families in this bucket:

- `BOYWTW=46`
- `VEHTRF=4`
- `ARCSLN=2`
- `BCNSPP=1`
- `DAYMAR=1`

Representative rows:

| Row ID | Family | Representative asset/rule |
| --- | --- | --- |
| `ARCSLN|Area|Plain|7|32043` | `ARCSLN` | `ARCSLN01` |
| `ARCSLN|Area|Symbolized|347|32382` | `ARCSLN` | `ARCSLN01` |
| `BCNSPP|Point|Paper|1781|93806` | `BCNSPP` | `BCNCON81` |
| `DAYMAR|Point|Paper|2054|30314` | `DAYMAR` | `TOPSHP33` |
| `boywtw|Point|Paper|2601|30668` | `BOYWTW` | `BOYLAT54` |
| `boywtw|Point|Paper|2603|30670` | `BOYWTW` | `BOYLAT55` |
| `boywtw|Point|Paper|2605|30672` | `BOYWTW` | `BOYLAT56` |
| `boywtw|Point|Simplified|1347|31399` | `BOYWTW` | `BOYLAT53` |
| `boywtw|Point|Simplified|1348|31400` | `BOYWTW` | `BOYLAT52` |
| `vehtrf|Area|Plain|311|32346` | `VEHTRF` | `VEHTRF01` |

### `manual_overlay_residual` rows

Top families in this bucket:

- `OBSTRN=16`
- `RESARE=4`
- `BOYLAT=1`

Rows:

| Row ID | Family | Representative asset/rule |
| --- | --- | --- |
| `BOYLAT|Point|Paper|1886|93804` | `BOYLAT` | `BOYSPH79` |
| `OBSTRN|Area|Plain|121|32157` | `OBSTRN` | `FLTHAZ02` |
| `OBSTRN|Area|Plain|124|32160` | `OBSTRN` | `FLTHAZ02` |
| `OBSTRN|Area|Plain|126|32162` | `OBSTRN` | `FLTHAZ02` |
| `OBSTRN|Area|Symbolized|463|32498` | `OBSTRN` | `FLTHAZ02` |
| `OBSTRN|Area|Symbolized|466|32501` | `OBSTRN` | `FLTHAZ02` |
| `OBSTRN|Area|Symbolized|468|32503` | `OBSTRN` | `FLTHAZ02` |
| `OBSTRN|Point|Paper|2308|30465` | `OBSTRN` | `FLTHAZ02` |
| `OBSTRN|Point|Paper|2310|30467` | `OBSTRN` | `FLTHAZ02` |
| `OBSTRN|Point|Paper|2312|30469` | `OBSTRN` | `FLTHAZ02` |
| `OBSTRN|Point|Paper|2314|30471` | `OBSTRN` | `FLTHAZ02` |
| `OBSTRN|Point|Paper|2316|30473` | `OBSTRN` | `FLTHAZ02` |
| `OBSTRN|Point|Simplified|1190|31242` | `OBSTRN` | `FLTHAZ02` |
| `OBSTRN|Point|Simplified|1192|31244` | `OBSTRN` | `FLTHAZ02` |
| `OBSTRN|Point|Simplified|1194|31246` | `OBSTRN` | `FLTHAZ02` |
| `OBSTRN|Point|Simplified|1196|31248` | `OBSTRN` | `FLTHAZ02` |
| `OBSTRN|Point|Simplified|1198|31250` | `OBSTRN` | `FLTHAZ02` |
| `RESARE|Area|Plain|164|32200` | `RESARE` | `ESSARE01` |
| `RESARE|Area|Plain|165|32201` | `RESARE` | `PSSARE01` |
| `RESARE|Area|Symbolized|506|32541` | `RESARE` | `ESSARE01` |
| `RESARE|Area|Symbolized|507|32542` | `RESARE` | `PSSARE01` |

### `parser_or_compiler_spillover` rows

Top families in this bucket:

- `DAYMAR=6`
- `NEWOBJ=5`
- `_SLGTO=4`
- `RDOSTA=1`
- `TOWERS=1`

Rows:

| Row ID | Family | Representative asset/rule |
| --- | --- | --- |
| `DAYMAR|Point|Paper|2038|30307` | `DAYMAR` | `TOPSHP73TESOBJNAM21215110_1_1CHBLK21` |
| `DAYMAR|Point|Paper|2046|30312` | `DAYMAR` | `TOPSHP09TESOBJNAM21215110_1_1CHBLK21` |
| `DAYMAR|Point|Paper|2065|30320` | `DAYMAR` | `TOPSHP15TESOBJNAM21215110_1_1CHBLK21` |
| `DAYMAR|Point|Paper|2075|93761` | `DAYMAR` | `TOPSHP81TESOBJNAM21215110_1_1CHBLK21` |
| `DAYMAR|Point|Paper|2056|93775` | `DAYMAR` | `TOPSHP89TESOBJNAM21215110_1_1CHBLK21` |
| `DAYMAR|Point|Paper|2081|93958` | `DAYMAR` | `TOPSHPT8TESOBJNAM21215110_1_1CHBLK21` |
| `NEWOBJ|Area|Plain|119|32155` | `NEWOBJ` | `NEWOBJ01` |
| `NEWOBJ|Area|Symbolized|461|32496` | `NEWOBJ` | `NEWOBJ01` |
| `NEWOBJ|Line|Lines|775|31851` | `NEWOBJ` | `NEWOBJ01` |
| `NEWOBJ|Point|Paper|2307|30464` | `NEWOBJ` | `NEWOBJ01` |
| `NEWOBJ|Point|Simplified|1189|31241` | `NEWOBJ` | `NEWOBJ01` |
| `RDOSTA|Point|Simplified|1232|31284` | `RDOSTA` | `DGPS01DRFSTA01` |
| `TOWERS|Point|Paper|2469|93985` | `TOWERS` | `TOWERS74TXOBJNAM322151101_1CHBLK26` |
| `_slgto|Point|Paper|2978|31006` | `_SLGTO` | `BOYSPR03` |
| `_slgto|Point|Paper|2979|31007` | `_SLGTO` | `BOYSPR02` |
| `_slgto|Point|Simplified|1705|31757` | `_SLGTO` | `BOYSPR03` |
| `_slgto|Point|Simplified|1706|31758` | `_SLGTO` | `BOYSPR02` |

## Bucket-to-task mapping

| Bucket | Owning task | Why |
| --- | --- | --- |
| `snapshot_asset_available_but_not_reached` | `108-s52-all-lookuprow-closure` | This is compiler/loader/precedence work inside the existing snapshot path. |
| `supplemental_upstream_candidate` | `106-s52-upstream-supplemental-opencpn-assets` | These rows need a GPL-compatible supplemental upstream asset source before compiler closure can finish. |
| `manual_overlay_residual` | `107-s52-manual-overlay-asset-pack` | These rows match the manual-overlay residual list already called out in the Phase 6D plan. |
| `parser_or_compiler_spillover` | `108-s52-all-lookuprow-closure` | These are malformed/combined IDs or spillover families that should be normalized away in parser/compiler cleanup. |
| `harness_only_partial` | `109-s52-harness-and-standalone-proof` | The row is compiled already; only repository-owned graphical/reference/host evidence is missing. |
| `internal_or_meta_row_only` | `108-s52-all-lookuprow-closure` | These rows are not ordinary S57 objects, but they are still unsupported rows that task `108` must close before `110` can declare `3057/3057/0/0`. |
| `inventory_false_positive_or_misclassified` | `110-phase6d-all-lookuprow-verification` | Any surviving misclassification should block final verification. |

## Does the current `106-110` chain cover the backlog?

### Judgment

**Yes, but task wording needs one or two guardrails.**

### Evidence

The current audit found no uncovered bucket:

- `106` owns all `supplemental_upstream_candidate` rows
- `107` owns all `manual_overlay_residual` rows
- `108` owns all `parser_or_compiler_spillover` rows and all `internal_or_meta_row_only` rows
- `109` owns the dominant `harness_only_partial` backlog
- `110` can verify the chain once the earlier tasks eliminate those buckets

There is no leftover bucket that fundamentally falls outside `106-110`.

### Required guardrails

The current chain is sufficient only if these guardrails are made explicit in execution:

1. **Task `108` must explicitly own internal/meta rows.**  
   The current unsupported backlog is **exactly 288 internal/meta rows**. If task `108`
   only focuses on ordinary symbol rows, `110` cannot reach `unsupportedRows = 0`.

2. **Task `109` must be judged by inventory reason elimination, not just by a few new scenes.**  
   The largest bucket is `scene_harness_not_covered = 2650`. Task `109` must drive that
   reason to zero for ordinary rows, not just add a handful of proof scenes.

3. **Task `110` must report ordinary vs internal/meta splits separately and reject done when either side is non-zero.**  
   This prevents accounting tricks and ensures that ordinary S57 backlog is not hidden
   behind internal/meta cleanup, or vice versa.

## Risks

1. **Task `109` is the main risk center.**  
   `2650 / 3030` degraded rows are harness-only partials. If task `109` is interpreted
   too narrowly, Phase 6D will stall even if tasks `106-108` succeed.

2. **Task `108` must close unsupported internal/meta rows, not just ordinary spillover.**  
   Otherwise the chain cannot reach `supportedRows = 3057`.

3. **Task `106` still needs real supplemental-source proof.**  
   This audit classifies rows as `supplemental_upstream_candidate`, not “guaranteed
   present upstream.” Task `106` still needs a provenance-backed supplemental source
   manifest.

4. **No rows currently landed in `snapshot_asset_available_but_not_reached`.**  
   Under the strict snapshot-definition audit used here, that bucket is empty. If future
   work discovers hidden definition paths in the pinned snapshot, the audit should be rerun.

## Reproducibility

### Baseline source

- `tests/data/reference/phase6b_s52_resource_snapshot_inventory.reference.json`

### Audit script

- `scripts/phase6d_lookuprow_cause_bucket_audit.py`

### Commands

Build and run the existing committed inventory baseline test:

```powershell
cmake --build --preset build-windows-msvc-debug --target s52_resource_snapshot_inventory_tests --parallel 1
ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R '^runtime\.s52_resource_snapshot_inventory$' --output-on-failure
```

Regenerate the audit artifacts from the committed baseline:

```powershell
py -3 scripts/phase6d_lookuprow_cause_bucket_audit.py
```

Generated outputs:

- `docs/generated/phase6d_lookuprow_cause_bucket_inventory.json`
- `docs/generated/phase6d_lookuprow_cause_bucket_inventory.csv`

## Bottom line

- The current backlog is **`3030`**, not `3049`.
- The largest bucket is **`harness_only_partial`** with **`2650`** rows.
- The biggest Phase 6D risk center is **task `109`**, not `106` or `107`.
- The current `106-110` chain is **sufficient with guardrails**, especially:
  - `108` must own internal/meta closure
  - `109` must explicitly eliminate `scene_harness_not_covered`
  - `110` must verify ordinary and internal/meta splits separately
