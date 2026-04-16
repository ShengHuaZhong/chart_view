# Phase 4 Demo Verification

Date: 2026-04-16

## Purpose

This note records the focused Phase 4 verification baseline for `chart_view`.

Phase 4 in this repository means:
- runtime-owned PROJ-backed display projection
- projected coverage, chart ranking, quilt planning, and patch clipping
- S-52-backed S57 portrayal baseline
- runtime-owned Unicode and multilingual label flow

This note does not claim:
- S-64 compliance
- full S-52 coverage
- full ECDIS compliance
- full Phase 4 portrayal coverage for S-101

## Achieved baseline

The repository now demonstrates this runtime-owned Phase 4 baseline:

1. One shared display projection is selected inside `chart_runtime` and consumed by projected viewport, projected coverage, projected quilt planning, and projected label placement.
2. The projected quilt path keeps the known real S57 pair `C1511781.000` / `C1511782.000` alive in one quilt plan with active patch clipping instead of overview-only ownership.
3. The S57 portrayal path can execute a narrow S-52 baseline including:
   - presentation assets
   - lookup-driven instructions
   - display settings
   - conditional symbolization
4. The text path is Unicode-safe, supports runtime-owned font fallback and glyph caching, and can prefer `NOBJNM` over `OBJNAM` when real data provides national-language labels.
5. The repository has both:
   - a real-chart integrated smoke for projected quilt + S-52 + Unicode-capable labels
   - a narrow S-64-inspired reference behavior smoke subset

## Verification commands

Build:

```powershell
powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target projection_context_tests scene_builder_tests coverage_index_tests chart_selection_policy_tests quilt_planner_tests s57_quilt_smoke_tests s52_presentation_assets_tests portrayal_registry_tests s52_lookup_model_tests s52_display_settings_tests s52_conditional_symbology_tests feature_symbolizer_tests feature_renderer_tests unicode_text_tests font_fallback_tests glyph_cache_tests label_tests s64_reference_smoke_tests s57_symbolized_smoke_tests"
```

Focused Phase 4 matrix:

```powershell
powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(projection_context|scene_builder|coverage_index|chart_selection_policy|quilt_planner|s57_quilt_smoke|s52_presentation_assets|portrayal_registry|s52_lookup_model|s52_display_settings|s52_conditional_symbology|feature_symbolizer|feature_renderer|unicode_text|font_fallback|glyph_cache|label|s64_reference_smoke|s57_symbolized_smoke)' --output-on-failure"
```

Targeted real-pair evidence:

```powershell
& 'C:/Users/zsh/source/repos/chart_view/out/build/windows-msvc-debug/test/Debug/s57_quilt_smoke_tests.exe' '[targeted-pair]' -s --reporter console
```

## Focused Phase 4 matrix

| Area | Tests | Result |
| --- | --- | --- |
| Projection core | `runtime.projection_context` | Passed |
| Projected scene / coverage / ranking / quilt | `runtime.scene_builder`, `runtime.coverage_index`, `runtime.chart_selection_policy`, `runtime.quilt_planner`, `runtime.s57_quilt_smoke` | Passed |
| S-52 assets and instruction model | `runtime.s52_presentation_assets`, `runtime.portrayal_registry`, `runtime.s52_lookup_model`, `runtime.s52_display_settings`, `runtime.s52_conditional_symbology`, `runtime.feature_symbolizer` | Passed |
| Renderer integration | `runtime.feature_renderer`, `runtime.s57_symbolized_smoke`, `runtime.s64_reference_smoke` | Passed |
| Unicode / multilingual text | `runtime.unicode_text`, `runtime.font_fallback`, `runtime.glyph_cache`, `runtime.label` | Passed |

Matrix summary:
- 19/19 targeted Phase 4 tests passed on 2026-04-16.

## Fixed real-pair evidence

The targeted real-chart audit for:
- `C1511781.000`
- `C1511782.000`

produced these key results:

- `geographic relation: overlap`
- `projected relation: overlap`
- `coverage candidates: C1511781, C1511782`
- `ranked candidates: C1511782, C1511781`
- `quilt ordered chart ids: C1511782, C1511781`
- `quilt includes both target charts: 1`
- `chart A patchAreaSum=7838452328.951324`
- `chart B patchAreaSum=648448779.411289`
- `anyLayerWasClipped: true`
- combined `s52Hits=1086`
- combined `named=220`
- combined `unicodeNamed=69`
- combined `textCandidates=220`
- `visible projected labels: total=68 unicode=34`

These results are sufficient for the repository's Phase 4 baseline claim:
- projected quilting is active on real S57 data
- both charts survive into one runtime-owned quilt plan
- S-52-backed portrayal is active
- Unicode-capable labels are present on the real-chart path

## Environment notes

- Some direct runs may still print `pj_obj_create: Cannot find proj.db`.
- For the current repository baseline this was observed as a warning, not a blocker, because:
  - `ProjectionContext` remained valid in the tested flows
  - `runtime.s57_quilt_smoke` passed
  - `runtime.s64_reference_smoke` passed
- If future environments start failing `ProjectionContext` creation or projected-smoke execution, that should be treated as a real blocker.

## Remaining non-compliance scope

The following remain explicitly out of scope for this baseline:

- no claim of full S-64 pass / type approval
- no claim of full S-52 lookup or symbol coverage
- no claim of full ECDIS conditional-symbology behavior
- no claim of full S-101 Phase 4 portrayal parity
- no host-layer compliance baseline; Phase 4 remains runtime-first
- no statement that every deployment already has a fully configured PROJ data environment

## Conclusion

Phase 4 is complete at the repository's intended baseline:
- projected quilting works in runtime-owned display space
- S57 rendering uses a narrow S-52-backed portrayal path
- Unicode / multilingual labels work through the runtime-owned text pipeline
- the result is verified by both synthetic reference smoke and real-chart integrated smoke

This is a reproducible demo and engineering verification baseline, not a compliance declaration.
