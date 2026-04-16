# CM93 OpenCPN Capability Map

## Scope

This note compares the current `chart_view` CM93 runtime path against the CM93 implementation centered on OpenCPN's [`gui/src/cm93.cpp`](https://github.com/OpenCPN/OpenCPN/blob/master/gui/src/cm93.cpp). It is intentionally limited to the narrow question that currently blocks stronger CM93 extent generation in `chart_view`.

The goal of this round is not to reproduce OpenCPN's full CM93 subsystem. The goal is to identify the smallest runtime-only slice that makes `FeatureChartDataset.meta.extent` more trustworthy and gives future CM93 feature parsing a cleaner foundation.

## OpenCPN -> chart_view capability matrix

| Capability | OpenCPN file / class / function | OpenCPN responsibility | chart_view current file / function / type | Current status | Gap | This task action | Proposed target location in chart_view | Notes / risk |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Decrypt | `gui/src/cm93.cpp`: `CreateDecodeTable()`, `read_and_decode_bytes()`, `read_and_decode_int()` | Builds the decode table and reads decrypted CM93 payload bytes consistently across the parser | `src/runtime/cm93/cm93_decode.cpp`: `cm93Decrypt()` | `existing` | `chart_view` already moved from filename-XOR placeholder logic to the decode-table approach, but still uses only a small part of the decrypted payload | `defer` | Keep in `src/runtime/cm93/cm93_decode.cpp` | Good enough for extent hardening; no need to widen scope here |
| Dictionary | `gui/src/cm93.cpp`: `cm93_dictionary::LoadDictionary()`, `GetClassName()`, `GetAttrName()`, `GetAttrType()` | Resolves object-class and attribute dictionaries so raw feature records can become named chart objects | No equivalent; current reader only synthesizes `CM93_<classCode>` labels when features exist | `missing` | No dictionary loading, no class-name mapping, no attribute typing | `defer` | Future `src/runtime/cm93/cm93_dictionary.*` or equivalent internal helper | Not required to harden extents because header-driven extent can be improved without class dictionaries |
| Cell header | `gui/src/cm93.cpp`: `header_struct`, `Cell_Info_Block`, `read_header_and_populate_cib()` | Reads structured prolog/header facts, cell bounds, mercator bounds, counts, and transform inputs | `src/runtime/cm93/cm93_decode.hpp/.cpp`: `Cm93CellHeader`, `decodeCm93Cell()` | `partial` | Previous `chart_view` path only lifted a feature-count surrogate and a loose bbox; it did not preserve prolog lengths, mercator bounds, or transform facts as first-class structured fields | `implement` | `src/runtime/cm93/cm93_decode.hpp`, `src/runtime/cm93/cm93_decode.cpp` | Highest-impact slice for extent reliability; low architectural risk because it stays fully inside runtime internals |
| Feature record | `gui/src/cm93.cpp`: `read_feature_record_table()`, `CreateS57Obj()` | Parses feature records, ties them to geometry and attributes, and creates object instances | `src/runtime/cm93/cm93_decode.cpp`: `decodeCm93Cell()` currently clears `features` after header validation | `missing` | No actual feature-table parsing yet | `defer` | Future `src/runtime/cm93/cm93_decode.cpp` or split parser helpers | Full feature parsing is a much larger effort and not required for header-backed extent generation |
| Geometry | `gui/src/cm93.cpp`: `read_vector_record_table()`, `read_2dpoint_table()`, `read_3dpoint_table()`, `BuildGeom()` | Builds usable point/line/area geometry from CM93 vector and point tables | `src/runtime/cm93/cm93_decode.cpp`: `Cm93FeatureRecord.geometry`; `src/runtime/cm93/cm93_reader.cpp`: `convertToDataset()` | `partial` | Data model can carry geometry, but the decoder does not populate it yet | `defer` | Existing `src/runtime/cm93/cm93_decode.cpp` and `src/runtime/cm93/cm93_reader.cpp` | Geometry-based extents remain the preferred source, but header and fallback extents can be hardened first |
| Attributes | `gui/src/cm93.cpp`: `cm93_attr_block::GetNextAttr()`, attribute type helpers | Parses typed attribute blocks and special CM93 offsets such as `_wgsox` / `_wgsoy` | `src/runtime/cm93/cm93_decode.cpp`: `Cm93FeatureRecord.attributes`; `src/runtime/cm93/cm93_reader.cpp`: numeric/string attribute coercion | `missing` | There is a place to store attributes, but no actual attribute-block decoding | `defer` | Future CM93 attribute parser inside `src/runtime/cm93/` | Needed for richer semantics, not for the minimal extent hardening slice |
| Scale / offset / coordinate transform | `gui/src/cm93.cpp`: `Get_CM93_CellIndex()`, `Get_CM93_Cell_Origin()`, `Transform()` | Uses cell naming, scale bands, mercator bounds, and WGS84 offsets to transform CM93 coordinates into chart coordinates | `src/runtime/cm93/cm93_decode.cpp`: `cm93ScaleFactor()`, `cm93CellOrigin()`, `cm93ToLonLat()` | `partial` | Previous `cm93CellOrigin()` used a coarse filename heuristic inconsistent with OpenCPN; transform facts from header were not preserved or sanity-checked | `implement` | `src/runtime/cm93/cm93_decode.cpp`, `src/runtime/cm93/cm93_decode.hpp` | Safe narrow win: align cell-origin fallback with OpenCPN and preserve transform inputs without pretending to finish full geometry math |
| Coverage / cache | `gui/src/cm93.cpp`: `M_COVR_Desc`, `ProcessMCOVRObjects()`, `UpdateCovrSet()` | Builds multi-cell coverage and cache state for chart selection and quilting | No CM93-specific coverage/cache implementation; generic Phase 2 catalog/quilt code exists elsewhere in runtime but does not decode CM93 coverage objects | `missing` | No CM93-native coverage objects or cache derived from decoded cell content | `defer` | Future CM93 coverage helper under `src/runtime/cm93/` plus existing generic quilt pipeline | Out of scope for this round and explicitly Phase 2+ territory |

## Most important gaps affecting CM93 extent generation

1. `src/runtime/cm93/cm93_reader.cpp` previously treated geometry extent as the only real extent source.
2. `src/runtime/cm93/cm93_decode.cpp` previously discarded most structured header facts after decrypting the file.
3. `src/runtime/cm93/cm93_decode.cpp` previously used a filename heuristic for `cm93CellOrigin()` that did not match the OpenCPN cell-index logic.
4. `chart_view` had no explicit distinction between:
   - header bbox
   - decoded geometry bbox
   - filename/cell-grid fallback bbox
5. The current parser still does not decode CM93 features, so extent generation must remain honest about which source actually produced the bounds.

## Narrow implementation slice for this round

The smallest useful runtime-only enhancement is:

1. Preserve structured CM93 prolog/header facts in `Cm93CellHeader`.
2. Harden `decodeCm93Cell()` with table-length, file-length, count, and bbox sanity checks.
3. Preserve geographic and mercator bounds plus transform-rate/origin inputs instead of collapsing them into loose integers.
4. Align `cm93CellOrigin()` and cell-span fallback logic with the OpenCPN cell-index conventions.
5. Make `Cm93Reader` choose extent sources in a strict order:
   - decoded geometry extent
   - header geographic extent
   - cell-name fallback extent
6. Expose the chosen internal extent source through `Cm93ReadResult` for tests and debugging.

## Explicit deferrals

The following stay deferred on purpose:

1. Full feature-record parsing.
2. Full vector/point table parsing.
3. Dictionary loading and object-class naming.
4. Attribute-block decoding.
5. CM93-native coverage extraction and caching.
6. Any change in `chart_qtwidgets`, `chart_standalone`, renderer presentation, quilt policy, or portrayal logic.

## Verification hooks in this repo

The narrow slice above is intended to be verified by:

1. `test/runtime/cm93_reader_tests.cpp`
   - header/prolog sanity
   - malformed length rejection
   - header-driven extent selection
   - cell-name fallback extent selection
   - OpenCPN-aligned origin/span expectations
2. Existing real-data smoke behavior in `test/runtime/cm93_reader_tests.cpp`
   - if decode succeeds, the returned CM93 dataset should now have a valid extent even when there is still no decoded geometry
3. Existing SENC/quilt smoke targets
   - unchanged architectural boundary; improved extents simply give downstream runtime code better metadata to work with
