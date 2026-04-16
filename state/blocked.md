# Blocked

- None active as of 2026-04-17.

## Historical notes

- `67-s57-source-model-and-update-manifest`
  - Completed with no active blocker.
  - Verification evidence:
    - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target s57_reader_tests s57_senc_smoke_tests"`
    - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\\.(s57_reader|s57_senc_smoke)' --output-on-failure"`
  - Result:
    - `runtime.s57_reader` passed
    - `runtime.s57_senc_smoke` passed
  - Scope note:
    - task 67 added the internal S57 source-model and manifest foundation only
    - update application, SENC v2, and full S-52 work remain in later Phase 5 tasks

- `66-phase4-demo-verification`
  - Completed with no active blocker.
  - Verification evidence:
    - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target projection_context_tests scene_builder_tests coverage_index_tests chart_selection_policy_tests quilt_planner_tests s57_quilt_smoke_tests s52_presentation_assets_tests portrayal_registry_tests s52_lookup_model_tests s52_display_settings_tests s52_conditional_symbology_tests feature_symbolizer_tests feature_renderer_tests unicode_text_tests font_fallback_tests glyph_cache_tests label_tests s64_reference_smoke_tests s57_symbolized_smoke_tests"`
    - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(projection_context|scene_builder|coverage_index|chart_selection_policy|quilt_planner|s57_quilt_smoke|s52_presentation_assets|portrayal_registry|s52_lookup_model|s52_display_settings|s52_conditional_symbology|feature_symbolizer|feature_renderer|unicode_text|font_fallback|glyph_cache|label|s64_reference_smoke|s57_symbolized_smoke)' --output-on-failure"`
    - `C:/Users/zsh/source/repos/chart_view/out/build/windows-msvc-debug/test/Debug/s57_quilt_smoke_tests.exe '[targeted-pair]' -s --reporter console`
  - Result:
    - 19/19 targeted Phase 4 tests passed
    - the fixed real pair still passed as a non-skipped integrated runtime smoke
  - Scope note:
    - Phase 4 is complete at a repository demo / smoke baseline
    - this is not a compliance declaration

- `65-s64-reference-behavior-smoke`
  - Completed with no active blocker.
  - Verification evidence:
    - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target s64_reference_smoke_tests s57_quilt_smoke_tests"`
    - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(s64_reference_smoke|s57_quilt_smoke)' --output-on-failure"`
  - Environment note:
    - direct runs may still print `pj_obj_create: Cannot find proj.db`
    - for task 65 this remained a non-blocking environment warning because both `runtime.s64_reference_smoke` and `runtime.s57_quilt_smoke` passed
    - it would only become a blocker if `ProjectionContext` creation failed or the runtime smoke started failing
    - this task remains a smoke-level behavior subset, not a compliance claim

- `64-s52-unicode-real-chart-smoke-s57`
  - The projected quilt blocker was resolved by `64a-projected-quilt-unblock-for-known-real-pair`.
  - The real-parse semantic blocker was resolved by `64b-s57-semantic-baseline-unblock-for-real-smoke`.
  - The task was formally closed out after rerunning the fixed-pair integrated smoke for `C1511781.000` / `C1511782.000`.
  - Verification evidence:
    - `cmake --build --preset build-windows-msvc-debug --target s57_reader_tests s57_quilt_smoke_tests`
    - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.(s57_reader|s57_quilt_smoke)" --output-on-failure`
    - `C:/Users/zsh/source/repos/chart_view/out/build/windows-msvc-debug/test/Debug/s57_quilt_smoke_tests.exe '[targeted-pair]' -s --reporter console`
  - Result:
    - `runtime.s57_reader` passed
    - `runtime.s57_quilt_smoke` passed
    - the targeted pair audit passed and confirmed:
      - `quilt ordered chart ids: C1511782, C1511781`
      - `s52Hits=1086`
      - `named=220`
      - `unicodeNamed=69`
      - `textCandidates=220`
      - `visible projected labels: total=68 unicode=34`

- Historical note:
  - The previous `37-cm93-quilt-render-smoke` blocker was resolved by the CM93 runtime decode/extent hardening work in `src/runtime/cm93/`.
  - After the reader started preferring `geometry -> header -> cell-name fallback` extents and the OpenCPN-aligned cell-origin fallback was in place, `runtime.cm93_quilt_smoke` passed against the configured CM93 dataset root `C:/Users/zsh/Documents/chart_testdata/cm93`.
