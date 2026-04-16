# Phase 5 Demo Verification

Date: 2026-04-17

## Purpose

This note records the reproducible Phase 5 verification baseline for `chart_view`.

Phase 5 in this repository means:
- S57-first runtime-owned source/update ingest
- SENC v2 persistence for retained source semantics and update state
- a compiled private S-52 catalog, lookup path, and typed rule/instruction IR
- runtime-owned mariner settings, class filters, rule filters, and feature inspection APIs
- a broader real-chart S57 smoke baseline
- Qt host bindings that drive the runtime-owned Phase 5 controls without absorbing portrayal logic

This note does not claim:
- full S-52 coverage
- full S-64 compliance
- OpenCPN parity for every behavior
- full ECDIS compliance
- complete Phase 5 coverage for non-S57 sources

## Achieved baseline

The repository now demonstrates this Phase 5 S57-first baseline:

1. `chart_runtime` owns the S57 source/update ingest path, including update-manifest tracking and SENC v2 semantic persistence.
2. The runtime-owned S-52 path now includes:
   - compiled built-in catalog assets
   - stable rule ids
   - typed point/line/area/text/conditional instruction IR
   - runtime mariner settings and selection filters
3. The normal S57 render path can execute:
   - compiled lookup hits
   - conditional symbology
   - class/rule filtering
   - Unicode-capable text and projected label placement
4. The runtime-owned inspection surface can:
   - enumerate compiled S-52 rules
   - query features at a point
   - describe feature summaries and active-rule metadata
5. The Qt host layers can present and drive Phase 5 runtime controls without moving S-52, filtering, or text logic out of `chart_runtime`.

## Verification commands

Build:

```powershell
powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target s57_reader_tests senc_section_tests senc_writer_tests senc_reader_tests senc_v2_tests s52_presentation_assets_tests s52_catalog_compiler_tests s52_lookup_model_tests runtime_api_tests s52_conditional_symbology_tests feature_symbolizer_tests feature_renderer_tests s57_symbolized_smoke_tests s64_reference_smoke_tests s57_real_chart_smoke_tests chart_qtwidgets chart_standalone qtwidgets_smoke_tests"
```

Focused Phase 5 matrix:

```powershell
powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(s57_reader|senc_section|senc_writer|senc_reader|senc_v2|s52_presentation_assets|s52_catalog_compiler|s52_lookup_model|api|s52_conditional_symbology|feature_symbolizer|feature_renderer|s57_symbolized_smoke|s64_reference_smoke|s57_real_chart_smoke)|qtwidgets\.smoke|chart_standalone\.smoke|chart_standalone\.phase5_controls\.smoke' --output-on-failure"
```

Verbose real-chart smoke:

```powershell
powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -V -R '^runtime\.s57_real_chart_smoke$'"
```

Parity harness:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/opencpn_parity_harness.ps1 -Reference tests/data/parity/phase5_s57_fixed_pair.reference.json -Observation tests/data/parity/phase5_s57_fixed_pair.chart_view.json
```

## Focused Phase 5 matrix

| Area | Tests | Result |
| --- | --- | --- |
| S57 source/update ingest | `runtime.s57_reader` | Passed |
| SENC v2 format | `runtime.senc_section`, `runtime.senc_writer`, `runtime.senc_reader`, `runtime.senc_v2` | Passed |
| Compiled S-52 catalog and lookup | `runtime.s52_presentation_assets`, `runtime.s52_catalog_compiler`, `runtime.s52_lookup_model` | Passed |
| Runtime API / rule-selection surface | `runtime.api`, `runtime.feature_symbolizer` | Passed |
| Conditional + renderer execution | `runtime.s52_conditional_symbology`, `runtime.feature_renderer`, `runtime.s57_symbolized_smoke`, `runtime.s64_reference_smoke` | Passed |
| Broader real-chart S57 smoke | `runtime.s57_real_chart_smoke` | Passed |
| Host bindings | `qtwidgets.smoke`, `chart_standalone.smoke`, `chart_standalone.phase5_controls.smoke` | Passed |

Matrix summary:
- 18/18 focused Phase 5 tests passed on 2026-04-17.

## Real-chart evidence

The broader Phase 5 real-chart smoke passed on:
- `C1511781.000`
- `C1511782.000`
- `C1511783.000`

Verbose evidence:

- `C1511781`
  - `usageBand=3`
  - `features=534`
  - `s52Hits=183`
  - `named=86`
  - `unicodeNamed=48`
  - `textCandidates=86`
  - `visibleLabels=44`
  - `visibleUnicodeLabels=25`
- `C1511782`
  - `usageBand=4`
  - `features=1430`
  - `s52Hits=903`
  - `named=134`
  - `unicodeNamed=21`
  - `textCandidates=134`
  - `visibleLabels=63`
  - `visibleUnicodeLabels=13`
- `C1511783`
  - `usageBand=4`
  - `features=388`
  - `s52Hits=205`
  - `named=69`
  - `unicodeNamed=6`
  - `textCandidates=69`
  - `visibleLabels=45`
  - `visibleUnicodeLabels=2`

These results confirm that the broader S57 Phase 5 path works through:
- S57 reader and retained semantics
- SENC v2 write/read
- runtime-owned compiled S-52 lookup and rendering
- Unicode-capable label selection and visibility

## Fixed-pair parity evidence

The engineering parity harness for the fixed real pair:
- `C1511781.000`
- `C1511782.000`

passed with:
- observed chart ids: `C1511782`, `C1511781`
- `namedFeatures=220`
- `unicodeNamed=69`
- `textCandidates=220`
- `visibleLabels=68`

This remains an engineering cross-check only.
Normative truth still comes from:
- IHO S-52
- Annex A
- S-64

## Closeout notes

Task 81 did not add new runtime or host features.

The only code changes needed during final closeout were narrow test updates so the final matrix matched the current Phase 5 behavior:
- `test/runtime/feature_symbolizer_tests.cpp`
  - stopped assuming an old fixed instruction count for `DEPARE`
  - now asserts presence of the expected area instruction, conditional instructions, and text instruction
- `test/runtime/feature_renderer_tests.cpp`
  - restored a shared frame-color helper
  - aligned the patterned depth-area case with the current conditional `area/depth_shallow_pattern` path

## Environment notes

- Direct PROJ-backed runs still print `pj_obj_create: Cannot find proj.db` in this environment.
- In the current repository baseline this remains a warning, not a blocker, because:
  - the focused Phase 5 matrix passed
  - the verbose real-chart smoke passed
  - the fixed-pair parity harness passed
- If future environments start failing `ProjectionContext` creation or projected runtime smoke execution, that should be treated as a real blocker.

## Remaining non-compliance scope

The following remain explicitly outside this demo / verification baseline:
- no claim of complete S-52 asset or rule coverage
- no claim of full OpenCPN parity
- no claim of full S-64 pass / compliance
- no claim of full ECDIS compliance
- no claim of complete non-S57 Phase 5 parity
- no claim that every deployment already has a fully configured PROJ data environment

## Conclusion

Phase 5 is complete at the repository's intended S57-first demo / engineering baseline:
- runtime-owned S57 source/update ingest exists
- SENC v2 retains richer source semantics and update state
- runtime-owned compiled S-52 lookup/rule execution works on real S57 charts
- the runtime API exposes mariner settings, filters, and feature inspection
- the official host layers can drive those runtime-owned controls
- the result is backed by focused unit/smoke coverage, broader real-chart smoke, and an engineering parity harness

This is a reproducible repository baseline, not a compliance declaration.
