# Phase 6D All-Lookuprow Verification Baseline

## Scope

This note closes the **active Phase 6D task chain** as a verification baseline
only. It does **not** claim full S-52 completion, full S-64 completion, or
ECDIS compliance.

The note records:

- the current committed lookup-row baseline
- explicit per-bucket counts
- which active Phase 6D workstreams landed in the runtime-owned path
- which rows remain deferred, quarantined, or still outside the completed
  workstream chain

## Current committed lookup-row baseline

Source of truth:

- `tests/data/reference/phase6b_s52_resource_snapshot_inventory.reference.json`

Current committed counts:

| Metric | Count |
| --- | ---: |
| `lookupRowsTotal` | `3057` |
| `supportedRows` | `28` |
| `partialRows` | `2741` |
| `unsupportedRows` | `288` |
| `degradedRows` | `3029` |

This baseline is the current local working-tree truth after:

- `107-s52-manual-overlay-asset-pack`
- `107a-inland-current-symbols-vehtrf01`
- `108-s52-all-lookuprow-closure`
- `109-s52-harness-and-standalone-proof`

## Explicit bucket split

The degraded rows are reported as mutually exclusive buckets in this order:

1. `internal/meta`
2. `inland-current`
3. `legacy inland`
4. `residual compatibility`
5. `ordinary maritime`

Current split:

| Bucket | Degraded rows | Status |
| --- | ---: | --- |
| `ordinary maritime` | `2687` | active-workstream baseline established; still incomplete |
| `inland-current` | `3` | asset-closed, evidence still partial |
| `legacy inland` | `46` | deferred by decision gate |
| `residual compatibility` | `5` | quarantined |
| `internal/meta` | `288` | still separate explicit backlog |

Additional note:

- the residual compatibility IDs appear in **7** degraded rows overall
- `5` of those rows are counted in the explicit `residual compatibility` bucket
- `2` additional `DANGER53` occurrences currently sit inside the `internal/meta`
  bucket and remain counted there to avoid hiding internal/meta backlog

## What entered the runtime-owned path

### Pinned base snapshot

Primary fallback input source remains:

- `vendor/opencpn_s57data/Release_5.14.0/s57data`

This remains a machine-readable engineering input source only, not the
normative truth source.

### Supplemental OpenCPN resources

Current Phase 6D chain vendored:

- **none**

Task `106` closed the supplemental-upstream route honestly rather than claiming
resources that were not backed by stable upstream definitions.

### Repo-owned manual overlays

The current Phase 6D chain added these repo-owned overlays with provenance:

- maritime confirmed:
  - `ARCSLN01`
  - `NEWOBJ01`
- inland-current:
  - `VEHTRF01`

Current verification status of those asset families in degraded rows:

| Asset | Remaining degraded rows | Remaining reason |
| --- | ---: | --- |
| `ARCSLN01` | `2` | `scene_harness_not_covered` only |
| `NEWOBJ01` | `5` | `scene_harness_not_covered` only |
| `VEHTRF01` | `3` | `scene_harness_not_covered` only |

### Compiler-owned normalization

Task `108` moved the covered fallback naming noise into canonical IR tokens:

- `DGPS01DRFSTA01 -> RDOSTA02`
- `TOPSHP...TESOBJNAM... -> canonical TOPSHP*`
- `TOWERS74TXOBJNAM... -> TOWERS74`

### Runtime-owned CSP closure

Task `109` closed the covered conditional/runtime gap without widening the
public ABI:

- legacy/canonical alias acceptance on the runtime-owned conditional path:
  - `LIGHTS05 / LIGHTS06`
  - `SYMINS01 / SYMINS02`
  - `SOUNDG02 / SOUNDG03`
  - `DEPARE02 / DEPARE03`
  - `SLCONS03 / SLCONS04`
  - `OBSTRN04 / OBSTRN07`
  - `WRECKS02 / WRECKS05`
  - `RESARE02 / RESARE04`
- `NEWOBJ` / `SYMINS` fail-safe closure:
  - point -> `NEWOBJ01`
  - line -> `NEWOBJ01`
  - area -> `NEWOBJ01` + `LS_DASH_2_CHMGD`

## Bucket status

### Ordinary maritime

Status:

- **baseline established, not complete**

Current ordinary-maritime degraded rows: `2687`

Dominant remaining reasons:

- `scene_harness_not_covered = 2687`
- `compiler_missing_point_asset:FLTHAZ02 = 16`
- `compiler_missing_point_asset:ESSARE01 = 2`
- `compiler_missing_point_asset:PSSARE01 = 2`
- `compiler_missing_point_asset:BOYSPH79 = 1`
- `compiler_missing_point_asset:TOPSHP33 = 1`

Interpretation:

- the active Phase 6D workstreams improved the runtime-owned path
- they did **not** close the broader ordinary-maritime graphical-evidence or
  remaining asset backlog

### Inland-current

Status:

- **asset-closed, evidence still partial**

Current inland-current degraded rows: `3`

All current inland-current degraded rows are `VEHTRF01` rows whose remaining
reason is:

- `scene_harness_not_covered`

### Legacy inland

Status:

- **deferred**

Current legacy-inland degraded rows: `46`

Decision-gate IDs remain:

- `BOYLAT52`
- `BOYLAT53`
- `BOYLAT54`
- `BOYLAT55`
- `BOYLAT56`
- `BOYSPP50`

This bucket was intentionally kept out of the ordinary maritime Phase 6D work
chain.

### Residual compatibility

Status:

- **quarantined**

Current residual-compatibility degraded rows: `5`

Current explicit residual IDs in the ordinary residual bucket:

- `BCNCON81`
- `BOYSPR02`
- `BOYSPR03`

Additional note:

- `DANGER53` still appears, but only inside `internal/meta` rows in the current
  committed baseline and is therefore reported under that bucket

### Internal/meta

Status:

- **still separate explicit backlog**

Current internal/meta degraded rows: `288`

Top families:

- `$CSYMB = 178`
- `$AREAS = 54`
- `$LINES = 46`
- `$TEXTS = 5`
- `###### = 5`

Dominant reasons:

- `compiler_missing_lookup_row = 288`
- `scene_harness_not_covered = 288`

This bucket remains explicit and is not used to hide ordinary S57 backlog.

## Local reference status

The local word-processed PresLib reference was available during the active
Phase 6D chain:

- `docs/reference_local/S-52_PresLib_e4.0.0_Part_I_Clean_Draft.pdf`

It was used as a narrative behavior reference only. The repository's declared
higher-edition normative target remains higher than this local e4.0.0 document,
so this note does **not** claim final normative closure.

## Verification matrix

### Build

```powershell
powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target s52_catalog_compiler_tests s52_lookup_model_tests s52_conditional_symbology_tests feature_symbolizer_tests point_symbol_tests line_symbol_tests feature_renderer_tests s52_resource_snapshot_inventory_tests chart1_s64_reference_harness_tests s57_real_chart_smoke_tests qtwidgets_smoke_tests chart_standalone --parallel 1"
```

### Focused tests

```powershell
powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R '^(runtime\.s52_catalog_compiler|runtime\.s52_lookup_model|runtime\.s52_conditional_symbology|runtime\.feature_symbolizer|runtime\.point_symbol|runtime\.line_symbol|runtime\.feature_renderer|runtime\.s52_resource_snapshot_inventory|runtime\.chart1_s64_reference_harness|runtime\.s57_real_chart_smoke|qtwidgets\.smoke)$' --output-on-failure"
```

### Direct standalone host smoke

```powershell
C:/Users/zsh/source/repos/chart_view/out/build/windows-msvc-debug/apps/chart_standalone/Debug/chart_standalone.exe --open-chart C:/Users/zsh/Documents/chart_testdata/s57/C1511781.000 --chart-type s57 --smoke-test
```

### Result

- the focused Phase 6D verification matrix passed:
  - `runtime.s52_catalog_compiler`
  - `runtime.s52_lookup_model`
  - `runtime.s52_conditional_symbology`
  - `runtime.feature_symbolizer`
  - `runtime.point_symbol`
  - `runtime.line_symbol`
  - `runtime.feature_renderer`
  - `runtime.s52_resource_snapshot_inventory`
  - `runtime.chart1_s64_reference_harness`
  - `runtime.s57_real_chart_smoke`
  - `qtwidgets.smoke`
- the direct standalone host smoke on `C1511781.000` exited successfully
- the window remained visibly non-blank
- resize-center evidence remains covered by `qtwidgets.smoke`
- the presentation path remains:
  - `chart_runtime renderFrame -> copyFrameRgba -> QImage -> QPainter::drawImage`

## Bottom line

- the active Phase 6D task chain is complete as a **verification baseline**
- the runtime-owned compiler/CSP/manual-overlay workstreams landed without
  widening the ABI or moving logic into the host
- the repository still does **not** claim:
  - full S-52 completion
  - full S-64 completion
  - OpenCPN parity
  - ECDIS compliance
- remaining backlog is explicit, bucketed, and honest:
  - ordinary maritime remains incomplete
  - inland-current remains evidence-partial
  - legacy inland remains deferred
  - residual compatibility remains quarantined
  - internal/meta remains a separate explicit backlog
