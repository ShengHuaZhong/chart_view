# Phase 6A Verification

Date: 2026-04-17

## Purpose

This note records the reproducible Phase 6A verification baseline for `chart_view`.

Phase 6A in this repository means:
- a vendored OpenCPN `s57data` snapshot used as the engineering input bundle
- a `chartsymbols.xml`-first parser and richer internal S-52 source-model
- deterministic compilation of that resource bundle into `chart_view`-owned IR
- runtime-owned point / line / area / text / CSP execution for S57
- Chart 1 / selected S-64 graphical references owned by this repository
- an OpenCPN engineering delta harness on the same fixed scenes

This note does not claim:
- full S-64 pass or compliance
- full ECDIS compliance
- full OpenCPN parity
- full non-S57 portrayal parity
- that OpenCPN is the normative portrayal oracle

Normative truth remains:
- IHO S-52
- Annex A
- S-64

## Achieved baseline

The repository now demonstrates this Phase 6A baseline:

1. `chart_runtime` can ingest a pinned OpenCPN `s57data` snapshot as an engineering resource bundle without linking or embedding `s52plib`.
2. `chartsymbols.xml` is parsed into a richer internal source model that retains:
   - colour tables
   - lookups
   - point / line / area asset metadata
   - text instruction metadata
   - conditional instruction metadata
3. The richer source model is compiled into deterministic runtime-owned IR with stable rule ids and compiled conditional opcodes.
4. The runtime-owned portrayal path can execute that compiled IR through:
   - point-symbol rendering
   - line-style rendering
   - area-pattern rendering
   - text / annotation rendering
   - conditional symbology evaluation
5. The existing runtime API surface remains the control plane for:
   - mariner settings
   - class filters
   - rule filters
   - query / describe behavior
6. The repository now has fixed-scene graphical regression evidence for:
   - one Chart 1-inspired scene
   - two selected S-64-inspired scenes
7. The repository also has an engineering-only OpenCPN delta harness that makes remaining scene-level mismatches explicit without redefining normative pass/fail.

## Resource snapshot

The Phase 6A engineering input bundle is the vendored OpenCPN snapshot:

- `vendor/opencpn_s57data/Release_5.14.0/s57data/chartsymbols.xml`

That snapshot is used as:
- an engineering resource-format input
- a deterministic compiler input

It is not used as:
- a normative portrayal oracle
- a runtime dependency on OpenCPN internals
- a justification to link or embed `s52plib`

## Verification commands

Build:

```powershell
powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target opencpn_resource_bundle_tests opencpn_chartsymbols_parser_tests s52_instruction_string_parser_tests s52_catalog_compiler_tests s52_conditional_symbology_tests point_symbol_tests line_symbol_tests area_symbol_tests label_tests runtime_api_tests s64_reference_smoke_tests chart1_s64_reference_harness_tests s57_real_chart_smoke_tests s57_lookup_coverage_smoke_tests qtwidgets_smoke_tests chart_standalone --parallel 1"
```

Focused Phase 6A matrix:

```powershell
powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R '^(runtime\.api|assets\.opencpn_resource_bundle|runtime\.opencpn_chartsymbols_parser|runtime\.s52_instruction_string_parser|runtime\.s52_catalog_compiler|runtime\.s57_lookup_coverage_smoke|runtime\.s52_conditional_symbology|runtime\.point_symbol|runtime\.line_symbol|runtime\.area_symbol|runtime\.label|runtime\.s64_reference_smoke|runtime\.chart1_s64_reference_harness|runtime\.opencpn_visual_delta_harness|runtime\.s57_real_chart_smoke|qtwidgets\.smoke|chart_standalone\.phase5_controls\.smoke)$' --output-on-failure"
```

Verbose real-chart smoke:

```powershell
powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -V -R '^runtime\.s57_real_chart_smoke$'"
```

Manual OpenCPN engineering delta summary:

```powershell
powershell -ExecutionPolicy Bypass -NoProfile -File C:/Users/zsh/source/repos/chart_view/scripts/opencpn_visual_delta_harness.ps1 -ManifestDir C:/Users/zsh/source/repos/chart_view/tests/data/opencpn -ObservationDir C:/Users/zsh/source/repos/chart_view/tests/data/reference -OutDir C:/Users/zsh/source/repos/chart_view/out/build/windows-msvc-debug/opencpn_visual_delta_manual
```

## Focused Phase 6A matrix

| Area | Tests | Result |
| --- | --- | --- |
| Vendored OpenCPN resource bundle | `assets.opencpn_resource_bundle` | Passed |
| `chartsymbols.xml` parse + instruction parsing | `runtime.opencpn_chartsymbols_parser`, `runtime.s52_instruction_string_parser` | Passed |
| Compiled catalog / rule IR | `runtime.s52_catalog_compiler`, `runtime.s57_lookup_coverage_smoke` | Passed |
| Runtime API and settings surface | `runtime.api` | Passed |
| Conditional behavior | `runtime.s52_conditional_symbology`, `runtime.s64_reference_smoke` | Passed |
| Graphics engines | `runtime.point_symbol`, `runtime.line_symbol`, `runtime.area_symbol`, `runtime.label` | Passed |
| Repository-owned graphical references | `runtime.chart1_s64_reference_harness` | Passed |
| OpenCPN engineering delta | `runtime.opencpn_visual_delta_harness` | Passed |
| Broader real-chart regression | `runtime.s57_real_chart_smoke` | Passed |
| Host binding smoke | `qtwidgets.smoke`, `chart_standalone.phase5_controls.smoke` | Passed |

Matrix summary:
- 17/17 focused Phase 6A tests passed on 2026-04-17.

## Real-chart evidence

The broader Phase 6A real-chart smoke passed on:
- `C1511781.000`
- `C1511782.000`
- `C1511783.000`

Verbose evidence:

- `C1511781`
  - `usageBand=3`
  - `features=534`
  - `s52Hits=521`
  - `named=86`
  - `unicodeNamed=48`
  - `textCandidates=86`
  - `visibleLabels=43`
  - `visibleUnicodeLabels=8`
- `C1511782`
  - `usageBand=4`
  - `features=1430`
  - `s52Hits=1398`
  - `named=134`
  - `unicodeNamed=21`
  - `textCandidates=134`
  - `visibleLabels=72`
  - `visibleUnicodeLabels=6`
- `C1511783`
  - `usageBand=4`
  - `features=388`
  - `s52Hits=378`
  - `named=69`
  - `unicodeNamed=6`
  - `textCandidates=69`
  - `visibleLabels=52`
  - `visibleUnicodeLabels=1`

These results confirm that the broader S57 path works through:
- compiled OpenCPN-resource-derived lookup coverage
- runtime-owned conditional symbology
- point / line / area / text execution
- Unicode-capable label visibility on real charts

## Chart 1 / S-64 graphical reference evidence

The repository-owned graphical reference harness now fixes three scenes:
- `phase6a_chart1_day_standard`
- `phase6a_s64_traditional`
- `phase6a_s64_simplified`

Committed reference observations include:
- object-level rule/style assertions
- fixed crop hashes
- non-background pixel counts
- `sourceRcid`
- `tableName`
- `conditionIds`

This moves the main acceptance evidence beyond counter-only smoke thresholds.

## OpenCPN engineering delta status

The engineering-only delta harness currently reports:
- `totalScenes = 3`
- `totalComparableFeatures = 9`
- `totalFeatureDeltas = 8`
- `totalCropDeltas = 2`

Current notable deltas include:
- traditional-mode scenes still favoring simplified `RCID/tableName` selections for:
  - `BOYSPP`
  - `SOUNDG`
  - `WRECKS`
- Chart 1 `LNDARE` still missing the expected `OBJNAM` text-attribute linkage in the observed object-level output
- simplified-mode `BOYSPP` still selecting `BOYSPP02` instead of the curated OpenCPN expectation `BOYSPP11`
- simplified-mode `SOUNDG` still omitting the expected `SOUNDG02` condition id in the observed explain surface
- the selected `danger_symbol` and `sounding_symbol` crops still differ in two of the fixed scenes

These deltas are intentionally visible in the repository now.
They are engineering gaps, not normative failures by themselves.

## Environment notes

- Direct PROJ-backed runs still print `pj_obj_create: Cannot find proj.db` in this environment.
- For the current repository baseline this remains a warning, not a blocker, because:
  - the focused Phase 6A matrix passed
  - the broader real-chart smoke passed
  - the graphical reference harness passed
  - the OpenCPN engineering delta harness passed
- If future environments start failing `ProjectionContext` creation or the graphical/reference harnesses begin failing, treat that as a real blocker.

## Remaining gaps / non-compliance scope

The following remain explicitly outside this verification closeout:
- no claim of full OpenCPN parity
- no claim of full S-64 matrix pass
- no claim of full S-64 compliance
- no claim of full ECDIS compliance
- no claim of complete non-S57 portrayal parity
- no claim that the historical official raw Annex A ingest path is unblocked

The repository keeps the original `83-s52-annexa-asset-ingest-core` blocker as historical context.
Phase 6A closes the approved fallback line, not the raw official-asset acquisition problem.

## Conclusion

Phase 6A is complete at the repository's intended engineering baseline:
- a vendored OpenCPN `s57data` bundle is now compiled into `chart_view`-owned portrayal IR
- `chart_runtime` executes that IR through point / line / area / text / CSP engines
- the existing runtime API remains the control surface for settings, filters, and query behavior
- repository-owned Chart 1 / S-64 graphical regression now exists
- OpenCPN engineering deltas are repeatable and documented on the same fixed scenes

This is a reproducible S57-first full-graphics engineering baseline.
It is not a compliance declaration.
