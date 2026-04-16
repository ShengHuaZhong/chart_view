# Phase 5 Real-Chart Smoke Matrix

This note records the broader real-chart smoke requirements for Phase 5 S57 verification.

## Purpose

Task 79 expands the real-chart evidence beyond the narrow fixed pair baseline used earlier in Phase 4.

The goal is to prove that the fuller Phase 5 S57 path works on a broader configured real-chart sample set through:

- S57 reader
- SENC v2 write/read
- scene building
- runtime-owned S-52 symbolization and rendering
- projected label visibility checks

This is still a smoke matrix, not a compliance declaration.

## Required sample behavior

The smoke target:

- preserves the fixed pair:
  - `C1511781.000`
  - `C1511782.000`
- requires at least one additional readable `.000` chart beyond the fixed pair
- prefers additional charts with distinct estimated usage bands when available

If the configured dataset root does not provide at least:

- the fixed pair, and
- one additional readable chart,

the smoke target must gate/skip honestly.

## Verification target

Target:

- `runtime.s57_real_chart_smoke`

Command shape:

```powershell
cmake --build --preset build-windows-msvc-debug --target s57_real_chart_smoke_tests
ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "^runtime\.s57_real_chart_smoke$" --output-on-failure
```

## What the smoke asserts

For each selected sample chart, the smoke currently requires:

- readable real S57 input
- successful SENC v2 roundtrip with retained S57 source-model payload
- non-zero runtime render geometry
- non-background rendered output
- non-zero compiled S-52 symbol hits

Across the selected matrix, the smoke additionally requires:

- non-zero named features
- non-zero Unicode-capable names
- non-zero text-label candidates
- non-zero visible projected labels

## Boundary

This matrix proves the repository's current Phase 5 smoke baseline on real charts.

It does not claim:

- complete S-52 coverage
- OpenCPN parity for every ENC behavior
- S-64 compliance
- ECDIS compliance
