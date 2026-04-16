# Phase 4 S57 Semantic Unblock: `C1511781.000` / `C1511782.000`

## Scope

- Subtask: `64b-s57-semantic-baseline-unblock-for-real-smoke`
- Data root: `C:/Users/zsh/Documents/chart_testdata/s57`
- Fixed pair:
  - `C1511781.000`
  - `C1511782.000`
- Goal:
  - preserve the minimum real S57 semantics needed for the existing Phase 4 baseline
  - keep the fix inside `chart_runtime`
  - avoid turning this into full S-57 dictionary work or a broad S-52 lookup expansion

This subtask only hardens real-chart semantic retention. It does not change projected quilt ownership, host code, the runtime public ABI, or the breadth of the S-52 lookup table.

## Why The Real Pair Produced Zero Hits Before

The pre-fix targeted pair audit showed that the known real pair already reached the projected quilt path, but the combined semantic counters were all zero:

- `s52Hits = 0`
- `named = 0`
- `unicodeNamed = 0`
- `textCandidates = 0`

The direct cause was the way [s57_reader.cpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/s57/s57_reader.cpp) preserved parsed semantics:

- object classes were stored as placeholder acronyms like `OBJ30` instead of real S57 acronyms like `COALNE`
- parsed attributes were stored under placeholder keys like `A116` instead of semantic keys like `OBJNAM`
- `NATF` was not parsed at all, so national-language names were dropped before the runtime label path could see them

That blocked three existing Phase 4 consumers:

- [s52_lookup_model.hpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/portrayal/s52_lookup_model.hpp)
  - matches real S57 class acronyms such as `SOUNDG`, `DEPARE`, `COALNE`, `LIGHTS`
- [feature_symbolizer.cpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/portrayal/feature_symbolizer.cpp)
  - looks for real semantic keys such as `OBJNAM`
- [label_layout.hpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/label_layout.hpp)
  - selects multilingual label text through `NOBJNM -> OBJNAM`

The runtime did not need a full external dictionary to unblock this pair. It only needed to stop discarding the small slice of semantics already required by the current Phase 4 baseline.

## Narrow Fix

The fix is centered on two files:

- [s57_semantic_mapping.hpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/s57/s57_semantic_mapping.hpp)
- [s57_reader.cpp](/C:/Users/zsh/source/repos/chart_view/src/runtime/s57/s57_reader.cpp)

### 1. Preserve Real Baseline Object Acronyms

Added a small internal mapping table for the Phase 4 baseline classes that the current runtime already knows how to symbolize, including:

- `COALNE`
- `DEPARE`
- `DEPCNT`
- `DRGARE`
- `FAIRWY`
- `LIGHTS`
- `SOUNDG`
- `UWTROC`
- `WRECKS`

Unknown classes still fall back to the existing `OBJ<code>` placeholder. This keeps the change narrow and backward-compatible inside runtime internals.

### 2. Preserve The Name And Depth Attributes The Baseline Already Uses

Added a small semantic attribute mapping table for the keys the current runtime already consumes:

- `OBJNAM`
- `NOBJNM`
- `CATCOA`
- `DRVAL1`
- `DRVAL2`
- `VALDCO`
- `VALSOU`

Unknown attributes still fall back to `A<code>`.

### 3. Parse `NATF` In Addition To `ATTF`

The reader now parses both:

- `ATTF`
- `NATF`

That is the smallest reader change that allows national-language names to survive into the existing multilingual label-selection path.

## Why This Is The Narrowest Acceptable Fix

This subtask intentionally does not do any of the following:

- no full S-57 object dictionary
- no full S-57 attribute dictionary
- no broad S-52 rule-table expansion
- no filename-specific special cases for `C1511781` / `C1511782`
- no changes to projected quilt logic
- no font, glyph-cache, or host-layer changes

The change simply preserves a baseline semantic subset that the runtime already expects downstream.

## Real-Pair Results After The Fix

The targeted pair audit now reports:

| Chart | Features | Named | Unicode-capable named | Text candidates | S-52 baseline hits |
| --- | ---: | ---: | ---: | ---: | ---: |
| `C1511781` | `534` | `86` | `48` | `86` | `183` |
| `C1511782` | `1430` | `134` | `21` | `134` | `903` |
| Combined | `1964` | `220` | `69` | `220` | `1086` |

Additional real-pair evidence:

- `rawNobjnm = 216`
- `visible projected labels: total = 68`
- `visible projected labels: unicode = 34`

These counts show that the fix is not just surfacing metadata in isolation. The existing projected quilt + symbolization + label path is now consuming the preserved semantics through the runtime renderer.

## Why This Is Enough For The Phase 4 Baseline Smoke

Task 64 only requires an honest baseline smoke proving that:

- projected quilt rendering works on a real S57 pair
- S-52-backed symbolization produces visible output
- Unicode-capable label flow survives end-to-end when the real data supplies names

After this subtask:

- the pair still survives as a two-chart projected quilt from `64a`
- the same pair now yields non-zero S-52 lookup hits
- the same pair now yields non-zero `OBJNAM` / `NOBJNM`-driven label candidates
- the same integrated smoke now passes through `runtime.s57_quilt_smoke`

That is enough to remove the semantic blocker for task 64 without claiming full S-57 dictionary completeness.

## Deferred Work

Still intentionally deferred:

- broader S-57 object and attribute coverage outside the current Phase 4 baseline
- richer national-language handling beyond the existing `NOBJNM -> OBJNAM` selection path
- any compliance claim beyond the current smoke baseline

## Verification

Build:

```powershell
cmake --build --preset build-windows-msvc-debug --target s57_reader_tests s57_quilt_smoke_tests
```

Targeted runtime tests:

```powershell
ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.(s57_reader|s57_quilt_smoke)" --output-on-failure
```

Targeted real-pair audit:

```powershell
& 'C:/Users/zsh/source/repos/chart_view/out/build/windows-msvc-debug/test/Debug/s57_quilt_smoke_tests.exe' '[targeted-pair]' -s --reporter console
```

Observed result:

- `runtime.s57_reader` passed
- `runtime.s57_quilt_smoke` passed
- the targeted real-pair audit passed
- the targeted audit printed non-zero `s52Hits`, `named`, `unicodeNamed`, and `textCandidates` for the fixed real pair
