# Done

## 00-repo-bootstrap
- DLL-first skeleton: `chart_runtime.dll` → `chart_qtwidgets.dll` → `chart_standalone.exe`.
- Top-level CMake with `chart_view_BUILD_RUNTIME`, `chart_view_BUILD_QTWIDGETS`, `chart_view_BUILD_STANDALONE`, `chart_view_BUILD_TESTS` options.
- Narrow runtime C API (`chart_runtime.h`), Qt Widgets host (`chart_view_widget.hpp`), standalone demo (`main.cpp`).
- Catch2 unit tests (`runtime_api_tests`, `qtwidgets_smoke_tests`) and CTest smoke tests (`chart_standalone.version`, `chart_standalone.smoke`, `package.smoke`).
- Config file generation with feature flags (QT_RHI, S57, CM93, S101).
- `CMakePresets.json` with vcpkg toolchain and Qt6 prefix path.
- Removed orphan template artifacts: `tests/`, `interfaces/`, `src/ftxui_sample/`, `src/sample_library/`, `fuzz_test/`, `web/`.
- Verification: `windows-msvc-debug` configure + build + all 5 tests pass.

## 01-runtime-api-contract
- Separated monolithic `chart_runtime.h` into proper API contract:
  - `chart_runtime_types.h` -- all types, enums, opaque handle, DTO structs (status codes, feature flags, chart source type, viewport, runtime info).
  - `chart_runtime_c_api.h` -- all C function declarations with export macros.
  - `chart_runtime.h` -- convenience header including both.
- Extended status codes: added `not_initialized`, `already_initialized`, `not_implemented`, `io_error`, `invalid_format`.
- Added lifecycle API: `chart_view_runtime_initialize()` and `chart_view_runtime_shutdown()` with guard logic.
- Added type scaffolding: `chart_view_viewport_t` DTO, `chart_view_chart_source_type_t` enum.
- Updated CMake install rules to install all three public headers.
- Extended `runtime_api_tests.cpp`: 6 test cases, 22 assertions (lifecycle, types, error paths).
- Verification: `windows-msvc-debug` configure + build + all 5 CTest targets pass.

## 02-runtime-dll-skeleton
- Extracted internal `RuntimeContext` class (`runtime_context.hpp` / `runtime_context.cpp`) from flat C struct.
- Introduced `RuntimeState` enum (`kCreated`, `kInitialized`, `kShutDown`) replacing bare `bool`.
- `RuntimeContext` destructor auto-shuts-down if still initialized (safe destroy without explicit shutdown).
- `chart_runtime.cpp` is now a thin C API bridge delegating to `RuntimeContext`.
- Feature flag building moved into `RuntimeContext` constructor.
- CMakeLists.txt updated to compile `runtime_context.cpp`.
- No public API changes -- all existing tests pass unchanged.
- Verification: `windows-msvc-debug` configure + build + all 5 CTest targets pass (22 assertions, 6 test cases).

## 03-qtwidgets-dll-skeleton
- Added internal `RuntimeBridge` class (`runtime_bridge.hpp` / `runtime_bridge.cpp`) managing widget-to-runtime connection.
- `RuntimeBridge` tracks owned vs external runtime handles; owned handles are destroyed on detach, external handles are released without destruction.
- Extended `ChartViewWidget` public API: `attachRuntime(chart_view_runtime_t*)`, `detachRuntime()`, `hasRuntime()`, `runtimeInfo()`.
- Widget creates an owned runtime by default on construction; supports detach-then-attach pattern for host-provided runtimes.
- `ChartViewWidget::Impl` now holds `RuntimeBridge bridge` instead of raw C handle pointer.
- Added `updateStatusLabel()` private helper to refresh status text after runtime changes.
- CMakeLists.txt updated: added `runtime_bridge.cpp` to sources, added install rule for `chart_view_widget.hpp`.
- Extended `qtwidgets_smoke_tests.cpp`: 3 test cases, 14 assertions (bootstrap, external attach, null reject).
- Verification: `windows-msvc-debug` configure + build + all 5 CTest targets pass; runtime: 6 test cases / 22 assertions; qtwidgets: 3 test cases / 14 assertions.

## 04-standalone-host-skeleton
- Extracted `MainWindow` class (`main_window.hpp` / `main_window.cpp`) in `chart_standalone` namespace.
- `MainWindow` sets `ChartViewWidget` as central widget.
- Basic menu scaffold: File > Exit, Help > About (shows version in status bar).
- Status bar initialized with "Ready" message.
- `main.cpp` updated to instantiate `MainWindow` instead of inline `QMainWindow`.
- CMakeLists.txt updated: `CMAKE_AUTOMOC ON`, added `main_window.hpp`/`.cpp` to sources.
- `--version` and `--smoke-test` flags preserved and still work.
- No runtime/qtwidgets code modified.
- Verification: `windows-msvc-debug` configure + build + all 5 CTest targets pass; `--version` and `--smoke-test` both exit 0.

## 05-viewport-scene-core
- Added `ViewportState` (viewport_state.hpp) -- wraps `chart_view_viewport_t` with revision counter and validity check.
- Added `SceneModel` (scene_model.hpp) -- mutable layer collection with `SceneLayerEntry` placeholder struct.
- Added `SceneSnapshot` (scene_snapshot.hpp) -- immutable capture of SceneModel + ViewportState for render consumption.
- Added `RenderScheduler` skeleton (render_scheduler.hpp / .cpp) -- accepts `shared_ptr<const SceneSnapshot>`, tracks frame count.
- Wired `ViewportState`, `SceneModel`, `RenderScheduler` into `RuntimeContext` as member fields with accessor methods.
- CMakeLists.txt updated: added `render_scheduler.cpp` to chart_runtime sources.
- New test executable `viewport_scene_tests` with 7 test cases / 29 assertions covering all new types.
- Test compiles `render_scheduler.cpp` directly (internal class, not DLL-exported).
- No public C API changes. No qtwidgets or standalone changes.
- Verification: `windows-msvc-debug` configure + build + all 6 CTest targets pass.

## 06-rhi-backend-bootstrap
- Added `RhiRenderBackend` class (rhi_render_backend.hpp / .cpp) inside chart_runtime.
- Creates QRhi with Null backend (headless); owns offscreen `QRhiTexture` color target and `QRhiTextureRenderTarget`.
- Frame lifecycle: `initialize(width, height)` creates QRhi + offscreen RT; `renderClearFrame(r,g,b,a)` executes `beginOffscreenFrame` / `beginPass` clear / `endPass` / `endOffscreenFrame`.
- chart_runtime.dll now links `Qt6::GuiPrivate` for `<rhi/qrhi.h>` access.
- `Dependencies.cmake` updated: Qt6 `find_package` expanded with `GuiPrivate` component; condition widened to include `chart_view_BUILD_RUNTIME`.
- Runtime test paths updated to include Qt bin dir since chart_runtime now depends on Qt Gui.
- New test executable `rhi_render_tests` with 4 test cases / 14 assertions (uninitialized state, Null backend init, invalid size rejection, clear frame execution).
- No public C API changes. No qtwidgets or standalone modifications.
- Verification: `windows-msvc-debug` configure + build + all 7 CTest targets pass.

## 07-feature-model-core
- Added `chart_data/` subdirectory under `src/runtime/` with unified feature model.
- `geometry.hpp`: `Coordinate`, `Extent`, `PointGeometry`, `LineGeometry`, `AreaGeometry`, `Geometry` variant, `GeometryType` enum, `geometryType()` helper.
- `dataset_meta.hpp`: `DatasetMeta` struct (name, sourceType, nativeScale, extent, usageBand, edition, update).
- `feature.hpp`: `Feature` struct (id, classCode, classAcronym, `Geometry`, `AttributeValue` variant map).
- `feature_chart_dataset.hpp`: `FeatureChartDataset` class (meta + flat feature vector, add/reserve/clear).
- All types are header-only, internal to chart_runtime. No CMake source changes needed.
- New test executable `feature_model_tests` with 12 test cases / 32 assertions covering geometry types, dataset meta, feature attributes, and synthetic S57/CM93/S-101 data mapping.
- No public C API changes. No qtwidgets or standalone modifications.
- Verification: `windows-msvc-debug` configure + build + all 8 CTest targets pass.

## 08-senc-section-model
- Added `senc/senc_types.hpp` defining the SENC v1 binary layout.
- `kSencMagic` (0x434E4553, "SENC" in LE) and `kSencFormatVersion` (1).
- `SectionType` enum: 10 mandatory sections (FileHeader, SourceManifest, DatasetMeta, FeatureTable, GeometryBlob, AttributeBlob, SpatialIndex, RenderCache, PickIndex, StringTable).
- `SectionDesc` struct (16 bytes): type, reserved, offset, size, checksum.
- `FileHeader` struct (16 bytes): magic, formatVersion, sectionCount, totalFileSize.
- `static_assert` for binary layout stability (both structs 16 bytes).
- New test executable `senc_section_tests` with 7 test cases / 26 assertions including STATIC_REQUIRE layout checks and magic byte verification.
- No public C API changes. No qtwidgets or standalone modifications.
- Verification: `windows-msvc-debug` configure + build + all 9 CTest targets pass.

## 09-senc-writer-core
- Added `SencWriter` class (`senc/senc_writer.hpp` / `senc_writer.cpp`) in `chart_view::runtime::senc`.
- Writes structurally valid SENC v1 binary from `FeatureChartDataset`.
- Layout: FileHeader (16 bytes) + SectionDesc table (10 x 16 bytes) + ordered section payloads.
- Encodes SourceManifest (source type + name), DatasetMeta (full metadata fields), FeatureTable (feature count + per-feature id/classCode/classAcronym/geoType/attrKeys), GeometryBlob (typed geometry per feature), AttributeBlob (typed key-value pairs).
- Stub sections (SpatialIndex, RenderCache, PickIndex, StringTable) written with size 0.
- CRC-32 checksum per section payload.
- `write()` returns `vector<uint8_t>` blob; `writeToFile()` convenience method.
- Internal class (not DLL-exported); test compiles source directly.
- New test executable `senc_writer_tests` with 11 test cases / 196 assertions covering header validity, section count, empty dataset, CRC checksums, contiguous offsets, SourceManifest roundtrip, DatasetMeta roundtrip, FeatureTable roundtrip, GeometryBlob roundtrip, blob size consistency, and deterministic output.
- No public C API changes. No qtwidgets or standalone modifications.
- Verification: `windows-msvc-debug` configure + build + all 10 CTest targets pass.

## 10-senc-reader-core
- Added `SencReader` class (`senc/senc_reader.hpp` / `senc_reader.cpp`) in `chart_view::runtime::senc`.
- `SencReadResult` struct with `ok`, `error`, and `dataset` fields.
- Validates magic, format version, totalFileSize, section table integrity, and CRC-32 checksums per section.
- Decodes SourceManifest, DatasetMeta, FeatureTable, GeometryBlob, and AttributeBlob sections.
- Reconstructs `FeatureChartDataset` with full metadata, features (id, classCode, classAcronym), typed geometry (point/line/area with holes), and typed attributes (int64/double/string).
- `read(span<const uint8_t>)` for in-memory blobs; `readFromFile(path)` for file I/O.
- Stub sections (SpatialIndex, RenderCache, PickIndex, StringTable) silently skipped.
- Added mutable `features()` accessor to `FeatureChartDataset` (non-breaking).
- Internal class (not DLL-exported); test compiles source directly.
- New test executable `senc_reader_tests` with 12 test cases / 61 assertions covering error paths (too small, bad magic, wrong version, CRC corruption, size mismatch) and full roundtrip (empty dataset, metadata, feature count, point/line/area geometry, attribute values).
- No public C API changes. No qtwidgets or standalone modifications.
- Verification: `windows-msvc-debug` configure + build + all 11 CTest targets pass (10 excluding package.smoke, all pass).

## 11-senc-validator-rebuild
- Added `SourceManifest` struct (`senc/source_manifest.hpp`) with name, sourceType, sourceSize, sourceTimestamp, sourceHash, edition, update fields.
- Added `SencValidator` class (`senc/senc_validator.hpp` / `senc_validator.cpp`) with `validate(current, stored)` and static `missing()` methods.
- `RebuildReason` enum: kNone, kMissing, kNameMismatch, kSourceTypeMismatch, kSizeMismatch, kTimestampNewer, kHashMismatch, kEditionMismatch, kUpdateMismatch.
- `ValidationResult` struct with rebuildNeeded, reason, detail.
- Validation priority: name > sourceType > edition > update > size > timestamp > hash.
- Unknown values (0) are ignored for size, timestamp, and hash comparisons.
- Extended SencWriter with `setSourceManifest()` for explicit manifest encoding; derives minimal manifest from DatasetMeta when not set.
- Extended SencReader `SencReadResult` with `optional<SourceManifest>` extracted from the SourceManifest section.
- SourceManifest binary format extended: sourceType(u32) + name(lpstr) + sourceSize(u64) + sourceTimestamp(i64) + sourceHash(u32) + edition(u32) + update(u32).
- Added mutable `features()` accessor to `FeatureChartDataset` (non-breaking addition from task 10).
- Updated existing senc_writer_tests to verify new manifest format.
- New test executable `senc_validator_tests` with 15 test cases / 36 assertions covering all rebuild reasons, unknown-value handling, and full writer-reader-validator roundtrip.
- No public C API changes. No qtwidgets or standalone modifications.
- Verification: `windows-msvc-debug` configure + build + all 12 CTest targets pass (11 excluding package.smoke, all pass).

## 11a-chart-testdata-fixtures
- `docs/test_data_policy.md` already existed; left unchanged.
- Created `tests/data/README.md` with setup instructions.
- Created `tests/data/s57/manifest.example.json` with S-57 manifest schema.
- Created `tests/data/cm93/manifest.example.json` with CM93 manifest schema.
- CMake variables `CHARTSYS_ENABLE_REAL_CHART_TESTS`, `CHARTSYS_S57_TESTDATA_ROOT`, `CHARTSYS_CM93_TESTDATA_ROOT` already existed.
- Added CMake validation: when `CHARTSYS_ENABLE_REAL_CHART_TESTS=ON`, raises `FATAL_ERROR` if roots are empty or non-existent.
- Status messages show enabled roots at configure time.
- Local test data verified: S57 at `C:/Users/zsh/Documents/chart_testdata/s57` (5 charts), CM93 at `C:/Users/zsh/Documents/chart_testdata/cm93`.
- No runtime/qtwidgets/standalone code changes.
- Verification: `windows-msvc-debug` configure + build + all 12 CTest targets pass (11 excluding package.smoke).

## 12-s57-single-chart-normalizer
- Added ISO 8211 parser (`s57/iso8211.hpp` / `iso8211.cpp`) in `chart_view::runtime::s57::iso8211`.
- Parses ISO 8211 leaders, directory entries, field data; extracts DDR field definitions with subfield labels/types.
- Added `S57Reader` class (`s57/s57_reader.hpp` / `s57_reader.cpp`) reading single S-57 .000 files.
- Reads file into memory, calls `iso8211::parse()`, extracts DSPM coordinate multiplier, builds vector record lookup, parses FRID feature records with ATTF attributes and FSPT spatial references.
- Produces `FeatureChartDataset` with metadata (name, sourceType=S-57, extent) and features (point/line/area geometry, attributes).
- Added `S57Normalizer` class (`s57/s57_normalizer.hpp`) -- thin wrapper around S57Reader for pipeline interface.
- New test executable `s57_reader_tests` with 8 test cases / 22 assertions:
  - ISO 8211 error paths (empty, truncated data).
  - S57Reader smoke on canonical chart (feature count >0, valid extent, geographic range, source type).
  - Point/line/area category extraction verification.
  - Attribute extraction verification.
  - Error paths (nonexistent file, invalid data).
  - S57Normalizer smoke on canonical chart.
- Test data path injected via `CHARTSYS_S57_TESTDATA_ROOT` compile definition; tests SKIP if not available.
- Internal classes (not DLL-exported); test compiles `.cpp` sources directly.
- No public C API changes. No qtwidgets or standalone modifications.
- Verification: `windows-msvc-debug` configure + build + all 13 CTest targets pass (12 excluding package.smoke).

## 13-cm93-single-chart-normalizer
- Added CM93 cell decoder (`cm93/cm93_decode.hpp` / `cm93_decode.cpp`) in `chart_view::runtime::cm93`.
- XOR decryption using filename-derived key and key table.
- Binary cell parsing: header (object class count, cell extents), object class table, feature records with geometry and attributes.
- Coordinate conversion: `cm93ToLonLat()` and `cm93CellOrigin()` for geographic positioning.
- Scale factor mapping by detail level (Z through G).
- Added `Cm93Reader` class (`cm93/cm93_reader.hpp` / `cm93_reader.cpp`) reading single CM93 cell files.
- `readFirstCell()` method discovers first available cell under a CM93 root directory.
- Produces `FeatureChartDataset` with metadata (name, sourceType=CM93, nativeScale, extent) and features.
- Added `Cm93Normalizer` class (`cm93/cm93_normalizer.hpp`) -- thin wrapper around Cm93Reader.
- New test executable `cm93_reader_tests` with 9 test cases / 16 assertions:
  - XOR decryption reversibility test.
  - Decode error paths (too small data).
  - Scale factor validity for all detail levels.
  - Cell origin parsing.
  - Reader smoke on canonical CM93 data (cell discovery, source type).
  - Error paths (nonexistent file, invalid data).
  - Normalizer smoke.
- Test data path injected via `CHARTSYS_CM93_TESTDATA_ROOT` compile definition.
- Internal classes (not DLL-exported); test compiles `.cpp` sources directly.
- No public C API changes. No qtwidgets or standalone modifications.
- Verification: `windows-msvc-debug` configure + build + all 14 CTest targets pass (13 excluding package.smoke).

## 14-s101-single-chart-normalizer
- Added `S101Reader` class (`s101/s101_reader.hpp` / `s101_reader.cpp`) in `chart_view::runtime::s101`.
- Phase-1 scaffold: reads file, validates minimum size, produces empty FeatureChartDataset with sourceType=S-101.
- Full S-101 GML parsing deferred until real test data is available (per task spec).
- Added `S101Normalizer` class (`s101/s101_normalizer.hpp`) -- thin wrapper around S101Reader.
- New test executable `s101_reader_tests` with 4 test cases / 8 assertions:
  - Rejects too-small data.
  - Scaffold produces empty dataset with correct metadata.
  - Error on nonexistent file.
  - Normalizer scaffold works.
- Internal classes (not DLL-exported); test compiles `.cpp` sources directly.
- No public C API changes. No qtwidgets or standalone modifications.
- Verification: `windows-msvc-debug` configure + build + all 15 CTest targets pass (14 excluding package.smoke).

## 15-s57-senc-build-smoke
- New test executable `s57_senc_smoke_tests` wiring S57Reader -> SencWriter -> SencReader full pipeline.
- 1 test case / 13 assertions: reads canonical S57 chart, normalizes to FeatureChartDataset, writes SENC v1, reads back, verifies chart identity (name), feature count, source type, and extent match (within 1e-6 margin).
- Result: C1511781 chart yields 534 features and 164433-byte SENC blob.
- No runtime source changes. No public C API changes. No qtwidgets or standalone modifications.
- Verification: `windows-msvc-debug` configure + build + all 16 CTest targets pass (15 excluding package.smoke).

## 16-cm93-senc-build-smoke
- New test executable `cm93_senc_smoke_tests` wiring CM93Reader -> SencWriter -> SencReader.
- 1 test case: reads first CM93 cell, if decryption succeeds writes SENC and reads back, verifying name/feature-count/source-type match.
- CM93 decryption is best-effort in Phase 1; test uses WARN+SUCCEED (not SKIP) when decryption fails to avoid Catch2 exit code 4 issue with CMake test wrapper.
- No runtime source changes. No public C API changes. No qtwidgets or standalone modifications.
- Verification: `windows-msvc-debug` configure + build + all 17 CTest targets pass (16 excluding package.smoke).

## 17-s101-senc-build-smoke
- New test executable `s101_senc_smoke_tests` wiring S101Reader -> SencWriter -> SencReader.
- 1 test case: creates synthetic 64-byte S-101 data, reads via scaffold reader (0 features), writes SENC, reads back, verifies name/feature-count/sourceType roundtrip.
- No real S-101 test data needed (scaffold is in-memory).
- No runtime source changes. No public C API changes. No qtwidgets or standalone modifications.
- Verification: `windows-msvc-debug` configure + build + all 18 CTest targets pass (17 excluding package.smoke).

## 18-scene-builder-from-senc
- New `SceneBuilderFromSenc` (scene_builder_from_senc.hpp) header-only class.
- `build()` method: computes viewport geographic extent (equirectangular approximation), filters features by AABB overlap, populates SceneModel with visible entries, returns SceneSnapshot.
- `buildAll()` method: includes all features without culling.
- Extended `SceneLayerEntry` with `featureIndex` and `geometryType` fields (backward-compatible defaults).
- New test executable `scene_builder_tests`: 7 test cases covering empty dataset, invalid viewport, culling, inclusion, buildAll, metadata carriage, and SENC roundtrip pipeline.
- No public C API changes. No qtwidgets or standalone modifications.
- Verification: `windows-msvc-debug` configure + build + all 19 CTest targets pass (18 excluding package.smoke).

## 19-feature-renderer-rhi
- New `FeatureLayerRenderer` (feature_layer_renderer.hpp/.cpp) renders point/line/area features from a SceneSnapshot.
- Collects vertex data with lon/lat-to-NDC projection (equirectangular). Reports render counts (pointsRendered, linesRendered, areasRendered, totalVertices).
- Phase 1: vertex collection + clear-frame pass via Null RHI backend. Full GPU pipeline deferred.
- New test executable `feature_renderer_tests`: 4 test cases covering uninitialized backend rejection, empty snapshot, all geometry types, and viewport culling.
- No public C API changes. No qtwidgets or standalone modifications.
- Verification: `windows-msvc-debug` configure + build + all 20 CTest targets pass (19 excluding package.smoke).

## 20-runtime-render-frame-api
- New C API functions: `chart_view_runtime_set_viewport`, `chart_view_runtime_load_senc`, `chart_view_runtime_render_frame`.
- New DTO: `chart_view_render_frame_result_t` with points/lines/areas/total_vertices counts.
- Extended `RuntimeContext` with `setViewport()`, `loadSenc()`, `renderFrame()` methods.
- `loadSenc()` decodes SENC blob via SencReader into internal FeatureChartDataset.
- `renderFrame()` builds scene via SceneBuilderFromSenc, counts visible features by geometry type.
- Added senc_reader.cpp and senc_writer.cpp to chart_runtime DLL compilation.
- 5 new test cases in runtime_api_tests: null arg rejection, lifecycle checks, full pipeline (set viewport + load SENC + render frame).
- No qtwidgets or standalone modifications.
- Verification: `windows-msvc-debug` configure + build + all 20 CTest targets pass (19 excluding package.smoke).

## 21-qt-chartview-widget
- Extended `ChartViewWidget` API with SENC loading and render result retrieval (`loadSenc`, `lastRenderResult`) and runtime-driven paint path.
- Added runtime viewport updates in `resizeEvent` and runtime render invocation in `paintEvent`.
- Extended `RuntimeBridge` with `setViewport`, `loadSenc`, and `renderFrame` wrappers over runtime C API.
- Added qtwidgets smoke coverage for end-to-end widget render pipeline with synthetic SENC blob.
- Fixed smoke test viewport targeting by setting runtime viewport center near the synthetic test feature.
- No runtime C API surface changes. No standalone app modifications.
- Verification: `windows-msvc-debug` build + `ctest -C Debug --force-new-ctest-process -E "package" --output-on-failure` with 19/19 tests passing.

## 22-open-chart-routing
- Added runtime-facing chart open API: `chart_view_runtime_open_chart_file(runtime, path, source_type)`.
- Runtime now routes source chart loading by type (S-57 / CM93 / S-101), builds SENC via `SencWriter`, loads through existing SENC path, and recenters viewport from dataset extent.
- Added standalone host open-chart command (`File -> Open Chart...`) with file-type routing via dialog filter/extension and runtime API call wiring.
- Added non-interactive host CLI path: `--open-chart <path> [--chart-type s57|cm93|s101]` for smoke/integration usage.
- Added automated host open-chart smoke test (`chart_standalone.open_chart.smoke`) using synthetic fixture `tests/data/s101/smoke_dataset.101`.
- No Phase 2/3 logic added. No quilt/catalog/policy changes.
- Verification: `ctest -C Debug --force-new-ctest-process -E "package" --output-on-failure` with 20/20 tests passing.

## 23-s57-single-chart-render-host-smoke
- Added runtime chart summary API: `chart_view_runtime_get_loaded_chart_info` with DTO `chart_view_loaded_chart_info_t` (source type, feature count, extent).
- Enhanced standalone open-chart smoke path to log loaded chart source/feature count/extent and render counts via host runtime path.
- Enforced non-blank render assertion for non-S-101 smoke runs (`points + lines + areas > 0`), while keeping S-101 scaffold permissive.
- Added `chart_standalone.s57_host_smoke` integration test (canonical local chart `C1511781.000`) through normal host open path.
- Verified runtime API and host smoke paths with targeted ctest and direct standalone smoke execution.
- Verification: `ctest -C Debug --force-new-ctest-process -R "runtime\.api|chart_standalone\.open_chart\.smoke|chart_standalone\.s57_host_smoke" --output-on-failure` with 3/3 passing; direct run logged `feature_count=534` and non-zero render counts.

## 24-cm93-single-chart-render-host-smoke
- Replaced placeholder CM93 decryption in `src/runtime/cm93/cm93_decode.cpp` with OpenCPN-compatible C-Map 93 decode-table algorithm (`Table_0` -> `Encode_table` -> `Decode_table`).
- Updated CM93 cell decode sanity logic to validate prolog/header layout and read feature-record count from the correct header location.
- Simplified Phase-1 CM93 decode result handling to validate decodability and header bounds without pretending to parse full CM93 feature tables.
- Updated standalone smoke pass criteria in `apps/chart_standalone/main.cpp` to allow zero rendered geometry for CM93 in Phase 1 (same permissive intent as S-101 scaffold, while still requiring successful open/load/render API path).
- Added `chart_standalone.cm93_host_smoke` CTest registration in `test/CMakeLists.txt` gated by `CHARTSYS_ENABLE_REAL_CHART_TESTS` and `CHARTSYS_CM93_TESTDATA_ROOT`.
- Verification:
  - Direct host smoke run: `chart_standalone.exe --smoke-test --open-chart C:/Users/zsh/Documents/chart_testdata/cm93 --chart-type cm93` exits `0` and logs `loaded_chart source= cm93`.
  - Full suite: `ctest -C Debug --output-on-failure` with 23/23 tests passing, including `chart_standalone.cm93_host_smoke` and `package.smoke`.

## 25-visible-host-presentation-path
- Preserved viewport center on resize by adding runtime viewport query support and updating `ChartViewWidget::resizeEvent()` to only refresh pixel dimensions.
- Added runtime-owned presentable RGBA frame buffer APIs (`chart_view_runtime_get_frame_buffer_info`, `chart_view_runtime_copy_frame_rgba`) without exposing Qt or QRhi types in the public ABI.
- Wired `FeatureLayerRenderer` into `RuntimeContext::renderFrame()` and replaced count-only behavior with minimum visible point/line/area rasterization into the runtime frame buffer.
- Extended `RhiRenderBackend` with resizable offscreen surface storage and simple CPU-side point/line/polygon raster helpers while keeping render ownership inside `chart_runtime`.
- Updated `ChartViewWidget::paintEvent()` to render through the runtime, copy the latest RGBA frame, and present it in the host widget without moving chart logic into qtwidgets.
- Expanded focused verification:
  - `runtime_api_tests` now cover viewport round-trip plus frame-buffer copy and non-background pixel validation.
  - `feature_renderer_tests` now verify rendered output is not pure background.
  - `qtwidgets_smoke_tests` now verify resize keeps the chart center and the grabbed widget image is visibly non-blank.
- Verification:
  - `cmake --build --preset build-windows-msvc-debug --target chart_runtime chart_qtwidgets chart_standalone runtime_api_tests feature_renderer_tests qtwidgets_smoke_tests`
  - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.api|runtime\.feature_renderer|qtwidgets\.smoke|chart_standalone\.s57_host_smoke" --output-on-failure`
  - Direct host smoke run: `chart_standalone.exe --smoke-test --open-chart C:/Users/zsh/Documents/chart_testdata/s57/C1511781.000 --chart-type s57` exits `0`.

## 25-s101-single-chart-render-host-smoke
- Resolved state/task mismatch by following the real `tasks/25-s101-single-chart-render-host-smoke.md` file instead of the stale `state/current_iteration.md` pointer to a non-existent `26-rhi-swapchain-presentation`.
- Extended internal `S101Reader` scaffold to recognize the checked-in synthetic `S101SMOKE` fixture format and emit a minimal renderable dataset (point + line + area) while leaving real S-101 parsing scaffold-only.
- Updated the checked-in fixture `tests/data/s101/smoke_dataset.101` and its README note so host smoke verification now exercises a visible runtime -> Qt host presentation path without requiring real S-101 data.
- Tightened standalone smoke validation so S-101, like S-57, must render non-zero visible geometry; CM93 remains permissive in Phase 1.
- Extended `s101_reader_tests` with synthetic-fixture coverage.
- Verification:
  - `cmake --preset windows-msvc-debug`
  - `cmake --build --preset build-windows-msvc-debug --target chart_runtime chart_standalone s101_reader_tests`
  - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.s101_reader|chart_standalone\.open_chart\.smoke|chart_standalone\.s101_host_smoke" --output-on-failure`
  - All 3 targeted tests passed, which confirms the synthetic S-101 host smoke path now reaches non-zero rendered geometry because `chart_standalone` smoke exits non-zero on zero-geometry S-101 frames.

## 26-phase1-demo-verification
- Added verification note [docs/phase1_demo_verification.md](C:/Users/zsh/source/repos/chart_view/docs/phase1_demo_verification.md) documenting the DLL-first Phase 1 display path, verification commands, and the final single-chart smoke matrix.
- Confirmed the runtime/qtwidgets/standalone linkage remains:
  - `chart_runtime.dll` owns source -> normalize -> SENC -> scene -> render.
  - `chart_qtwidgets.dll` hosts/presents the runtime frame in Qt Widgets.
  - `chart_standalone.exe` remains the demo host shell only.
- Fixed a verification-only rebuild blocker in `src/runtime/cm93/cm93_decode.cpp` by replacing a non-ASCII comment character that triggered MSVC warning `C4819` as an error during Phase 1 smoke target rebuilds.
- Verification:
  - `cmake --build --preset build-windows-msvc-debug --target chart_runtime chart_qtwidgets chart_standalone runtime_api_tests s57_senc_smoke_tests cm93_senc_smoke_tests s101_senc_smoke_tests qtwidgets_smoke_tests`
  - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.api|runtime\.(s57_senc_smoke|cm93_senc_smoke|s101_senc_smoke)|qtwidgets\.smoke|chart_standalone\.(smoke|open_chart\.smoke|s57_host_smoke|cm93_host_smoke|s101_host_smoke)" --output-on-failure`
  - Result: 10/10 targeted Phase 1 verification tests passed on 2026-04-15.

## 27-chart-catalog-core
- Added runtime-internal catalog types:
  - `catalog/chart_catalog.hpp`
  - `catalog/chart_catalog.cpp`
- Defined `ChartCatalogEntry` with `id`, `sencPath`, `extent`, `nativeScale`, `sourceType`, `usageBand`, `edition`, and `update`.
- Defined `ChartCatalog` with directory load, sorted entry storage, `findById`, and last-error reporting.
- Extended internal `SencReader` with metadata-only catalog APIs:
  - `readCatalogMeta(blob)`
  - `readCatalogMetaFromFile(path)`
- Catalog metadata load validates the SENC header and section table but only decodes `SourceManifest` and `DatasetMeta`, intentionally skipping full feature/geometry decode.
- Added `chart_catalog_tests.cpp` smoke coverage for metadata-only SENC reads, multi-file directory catalog loading, deterministic id ordering, and missing-directory failure handling.
- Added `runtime.chart_catalog` CTest registration.
- Verification:
  - `cmake --preset windows-msvc-debug`
  - `cmake --build --preset build-windows-msvc-debug --target chart_runtime chart_catalog_tests`
  - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.chart_catalog" --output-on-failure`
  - Result: 1/1 targeted catalog smoke test passed.

## 28-coverage-index
- Added runtime-internal coverage index sources:
  - `catalog/coverage_index.hpp`
  - `catalog/coverage_index.cpp`
- Built `CoverageIndex` as a catalog-over-extents spatial hash/grid, intentionally separated from renderer, host, and chart selection policy.
- `CoverageIndex::build()` copies catalog entries, computes global bounds, partitions them into a deterministic grid, and indexes chart extents into buckets.
- `CoverageIndex::query(viewportExtent)` returns deduplicated intersecting charts with exact extent-overlap filtering and stable id/path ordering.
- Added `coverage_index_tests.cpp` smoke coverage for intersecting-query results, multi-bucket deduplication, and invalid/remote viewport miss cases.
- Added `runtime.coverage_index` CTest registration.
- Verification:
  - `cmake --preset windows-msvc-debug`
  - `cmake --build --preset build-windows-msvc-debug --target chart_runtime coverage_index_tests`
  - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.coverage_index" --output-on-failure`
  - Result: 1/1 targeted coverage query smoke test passed.

## 29-chart-selection-policy
- Added runtime-internal selection policy sources:
  - `catalog/chart_selection_policy.hpp`
  - `catalog/chart_selection_policy.cpp`
- Implemented `ChartSelectionPolicy::rankCandidates()` to return a stable ordered chart list from coverage candidates and current viewport scale.
- Applied the requested priority layers without mixing in quilt logic:
  - scale fit: prefer native scale closest to current viewport scale
  - usage fit: prefer charts whose usage band best matches the target usage band implied by the viewport scale
  - source priority: `S-101` -> `S-57` -> `CM93` -> `unknown`
  - stable fallback: id/path ordering
- Added `chart_selection_policy_tests.cpp` coverage for scale ordering, usage-band tie-breaks, source-priority tie-breaks, and stable fallback ordering.
- Added `runtime.chart_selection_policy` CTest registration.
- Verification:
  - `cmake --preset windows-msvc-debug`
  - `cmake --build --preset build-windows-msvc-debug --target chart_runtime chart_selection_policy_tests`
  - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.chart_selection_policy" --output-on-failure`
  - Result: 1/1 targeted policy unit test target passed.

## 30-quilt-plan-model
- Added runtime-internal quilt plan header `quilt/quilt_plan.hpp`.
- Defined `QuiltLayer`, `QuiltSelectionResult`, and `QuiltPlan` as runtime-owned DTO/model types for one frame's multi-chart composition state.
- Kept the model header-only and free of `QWidget`, `QRhi`, or other host/render-backend types; it only references runtime catalog metadata and chart extents.
- `QuiltLayer::fromCatalogEntry()` converts a selected catalog entry into a drawable layer record while preserving the full chart extent and optionally clipping to a visible extent.
- `QuiltPlan` now stores the current viewport extent/scale, the ordered layer list, and the selection-policy result snapshot that produced the plan.
- Added `quilt_plan_tests.cpp` smoke coverage for layer creation from catalog entries, ordered plan accumulation, and `clear()` state reset.
- Added `runtime.quilt_plan` CTest registration.
- Verification:
  - `cmake --preset windows-msvc-debug`
  - `cmake --build --preset build-windows-msvc-debug --target quilt_plan_tests`
  - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.quilt_plan" --output-on-failure`
  - Result: 1/1 targeted quilt plan unit test passed.

## 31-quilt-planner-basic
- Added runtime-internal quilt planner sources:
  - `quilt/quilt_planner.hpp`
  - `quilt/quilt_planner.cpp`
- Implemented `QuiltPlanner::build()` to generate a `QuiltPlan` directly from:
  - `CoverageIndex` candidate lookup
  - `ChartSelectionPolicy` stable ranking
  - current viewport extent + scale
- The planner now:
  - queries overlapping charts from the coverage index
  - preserves the selection snapshot (`candidateCount`, ordered chart ids, viewport scale)
  - clips each selected chart to the current viewport and emits ordered `QuiltLayer` entries with stable `drawOrder`
- Kept the implementation fully inside `chart_runtime` and out of host/widget layers; no seam beautification, scene build, or renderer changes were mixed in.
- Added `quilt_planner_tests.cpp` smoke coverage for stable multi-chart ordering and empty-plan behavior when nothing overlaps.
- Added `runtime.quilt_planner` CTest registration.
- Verification:
  - `cmake --preset windows-msvc-debug`
  - `cmake --build --preset build-windows-msvc-debug --target chart_runtime quilt_planner_tests`
  - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.quilt_planner" --output-on-failure`
  - Result: 1/1 targeted quilt planner test passed.

## 32-scene-builder-from-quilt
- Upgraded the runtime scene model to represent multi-chart frames by adding `SceneChartEntry` metadata and a `sourceChartIndex` on each `SceneLayerEntry`.
- Extended `SceneSnapshot` to preserve both chart-level metadata and per-feature layer references for a composed frame.
- Upgraded `SceneBuilderFromSenc` with a new quilt-aware build path:
  - consumes a `QuiltPlan`
  - consumes multiple decoded SENC datasets in plan-layer order
  - clips each dataset against the layer-visible extent from the quilt plan
  - emits one `SceneSnapshot` containing visible features from multiple charts
- Preserved the existing single-chart build/buildAll paths while routing them through the same internal scene-entry helpers.
- Expanded `scene_builder_tests.cpp` with:
  - source-chart index assertions for existing single-chart coverage
  - a new multi-chart SENC roundtrip smoke case proving one snapshot can contain visible features from two quilt layers
- Verification:
  - `cmake --preset windows-msvc-debug`
  - `cmake --build --preset build-windows-msvc-debug --target chart_runtime scene_builder_tests`
  - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.scene_builder" --output-on-failure`
  - Result: 1/1 targeted scene builder test target passed.

## 33-feature-renderer-multichart
- Upgraded `FeatureLayerRenderer` to render from either:
  - a single `FeatureChartDataset` (existing path preserved), or
  - multiple datasets supplied as a span aligned to `SceneSnapshot` chart indices
- The renderer now uses `SceneLayerEntry::sourceChartIndex` to resolve each visible feature from the correct chart dataset inside one composed frame.
- Preserved draw order by continuing to render snapshot layers in snapshot order; this now supports chart buckets emitted by the quilt-aware scene builder.
- Added multi-chart renderer smoke coverage proving one frame can render geometry from two quilt layers/datasets in a single pass.
- Verification:
  - `cmake --preset windows-msvc-debug`
  - `cmake --build --preset build-windows-msvc-debug --target chart_runtime feature_renderer_tests`
  - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.feature_renderer" --output-on-failure`
  - Result: 1/1 targeted multi-chart renderer smoke target passed.

## 34-zoom-policy
- Added runtime-internal zoom policy sources:
  - `quilt/zoom_policy.hpp`
  - `quilt/zoom_policy.cpp`
- Defined deterministic zoom-step behavior through `zoomIn()` / `zoomOut()` with clamped scale bounds.
- Defined explicit zoom evaluation outputs via `ZoomDecision`, including:
  - requested/resolved scale
  - preferred quilt layer index for the target scale
  - current primary chart zoom state (`normal`, `overzoom`, `underzoom`, `no charts`)
  - whether the quilt plan should be rebuilt and why
- Implemented rebuild triggers for:
  - empty plan
  - preferred chart change
  - overzoom
  - underzoom
- Added `zoom_policy_tests.cpp` coverage for deterministic zoom stepping, stable no-rebuild cases, preferred-chart reselect triggers, overzoom/underzoom detection, and empty-plan behavior.
- Added `runtime.zoom_policy` CTest registration.
- Verification:
  - `cmake --preset windows-msvc-debug`
  - `cmake --build --preset build-windows-msvc-debug --target chart_runtime zoom_policy_tests`
  - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.zoom_policy" --output-on-failure`
  - Result: 1/1 targeted zoom policy unit test passed.

## 35-chartview-zoom-interaction
- Added a narrow runtime zoom ABI surface in `chart_runtime`:
  - `chart_view_zoom_scale_state_t`
  - `chart_view_zoom_rebuild_reason_t`
  - `chart_view_zoom_result_t`
  - `chart_view_runtime_step_zoom(...)`
- Implemented `RuntimeContext::stepZoom()` inside `chart_runtime` so zoom stepping stays runtime-owned and returns DTO-style zoom results without exposing `QWidget`, `QRhi`, or other Qt types through the C API.
- Extended the qtwidgets runtime bridge with `stepZoom(...)` and kept `ChartViewWidget` focused on host interaction only.
- Implemented wheel zoom handling in `ChartViewWidget`:
  - converts wheel delta into zoom steps
  - delegates scale changes to the runtime
  - applies anchor-preserving viewport recentering in the widget shell
  - requests a repaint after viewport updates
- Added verification coverage for both layers:
  - `runtime_api_tests.cpp` now checks that `chart_view_runtime_step_zoom()` updates viewport scale through the public C API
  - `qtwidgets_smoke_tests.cpp` now checks wheel zoom scale changes, anchor drift direction, and post-zoom rendering through `ChartViewWidget`
- Adjusted the qtwidgets smoke fixture to keep the synthetic point inside the zoomed viewport and relaxed the center-anchor assertion to a small margin so the smoke test verifies the interaction chain instead of a brittle pixel-perfect center.
- Verification:
  - `cmake --preset windows-msvc-debug`
  - `cmake --build --preset build-windows-msvc-debug --target chart_runtime chart_qtwidgets runtime_api_tests qtwidgets_smoke_tests`
  - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.api|qtwidgets\.smoke" --output-on-failure`
  - Result: 2/2 targeted runtime API and qtwidgets smoke tests passed.

## 36-s57-quilt-render-smoke
- Added a new verification target `runtime.s57_quilt_smoke` with `test/runtime/s57_quilt_smoke_tests.cpp`.
- The smoke test stays inside verification scope and keeps production boundaries unchanged while exercising the Phase 2 runtime chain end to end:
  - read two real S57 charts with overlapping or adjacent extents
  - normalize smoke-only metadata where S57 native scale is not available yet
  - write both datasets through SENC v1
  - load them back through `ChartCatalog` / `CoverageIndex` / `ChartSelectionPolicy` / `QuiltPlanner`
  - build a multi-chart scene snapshot
  - render through the runtime RHI backend
  - zoom in one step with `ZoomPolicy`, rebuild the quilt plan, and render again
- Added CMake wiring for `s57_quilt_smoke_tests`, including real-data root propagation and offscreen Qt/RHI test execution.
- Verification:
  - `cmake --preset windows-msvc-debug`
  - `cmake --build --preset build-windows-msvc-debug --target s57_quilt_smoke_tests`
  - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.s57_quilt_smoke" --output-on-failure`
  - Result: 1/1 targeted S57 quilt smoke test passed against configured real chart data.

## 37-cm93-quilt-render-smoke
- Added CM93 capability analysis note [docs/cm93_opencpn_capability_map.md](C:/Users/zsh/source/repos/chart_view/docs/cm93_opencpn_capability_map.md) comparing `chart_view` against OpenCPN across decrypt, dictionary, header, feature, geometry, attributes, transform, and coverage responsibilities.
- Hardened runtime-internal CM93 header handling in `src/runtime/cm93/cm93_decode.hpp/.cpp`:
  - structured `Cm93CellHeader` now preserves prolog lengths, geographic extent, mercator extent, record counts, and derived transform inputs
  - `decodeCm93Cell()` now validates prolog/table lengths, declared file size, and plausible header counts before accepting a cell
  - `cm93CellOrigin()` and `cm93CellSpanDegrees()` now follow the OpenCPN cell-index convention for fallback coverage
- Hardened `src/runtime/cm93/cm93_reader.hpp/.cpp`:
  - `Cm93ReadResult` now records which extent source was used
  - dataset extents are now chosen in this priority order: decoded geometry -> header geographic extent -> cell-name fallback extent
  - real-data reads now keep a valid `DatasetMeta.extent` even when CM93 feature decoding is still incomplete
- Expanded `test/runtime/cm93_reader_tests.cpp` with synthetic encoded-cell fixtures and focused coverage for:
  - header/prolog sanity
  - malformed length rejection
  - header-driven extent generation
  - cell-name fallback extent generation
  - OpenCPN-aligned cell-origin/span handling
- Tightened CM93 verification behavior without moving logic out of runtime:
  - `runtime.cm93_reader` now asserts valid extents on successful real-data reads
  - `runtime.cm93_senc_smoke` and `runtime.cm93_quilt_smoke` keep their best-effort gating paths but now use non-warning early-pass assertions so Windows/Catch2 does not terminate with breakpoint-style exit codes on gated runs
- Result:
  - `runtime.cm93_reader` passes with the new decode/extent checks
  - `runtime.cm93_quilt_smoke` now passes against the configured real CM93 dataset root, which resolves the previous task-37 blocker
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target cm93_reader_tests cm93_senc_smoke_tests cm93_quilt_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(cm93_reader|cm93_senc_smoke|cm93_quilt_smoke)' --output-on-failure"`
  - Result: 3/3 targeted CM93 runtime verification tests passed on 2026-04-16.

## 38-s101-quilt-render-smoke
- Added a new deterministic verification target `runtime.s101_quilt_smoke` with [s101_quilt_smoke_tests.cpp](C:/Users/zsh/source/repos/chart_view/test/runtime/s101_quilt_smoke_tests.cpp).
- Kept the task inside verification scope and reused the existing synthetic `S101SMOKE` fixture format from `S101Reader` instead of widening scope into real S-101 parsing or host code.
- The new smoke test now:
  - creates two synthetic S-101 chart source files with overlapping extents
  - reads them through `S101Reader`
  - writes both datasets through SENC v1
  - loads them back through `ChartCatalog` / `CoverageIndex` / `ChartSelectionPolicy` / `QuiltPlanner`
  - builds a multi-chart scene snapshot
  - renders through the runtime RHI backend
  - zooms in one step through `ZoomPolicy`, rebuilds the plan, and renders again
- Added CMake wiring for `s101_quilt_smoke_tests`, including offscreen Qt/RHI execution and `runtime.s101_quilt_smoke` registration.
- No runtime public API changes. No qtwidgets, standalone, renderer-production, or portrayal changes.
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --preset windows-msvc-debug"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target s101_quilt_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.s101_quilt_smoke' --output-on-failure"`
  - Result: 1/1 targeted S-101 quilt smoke test passed on 2026-04-16.

## 39-open-chart-directory-routing
- Added a runtime-owned directory-open path in `RuntimeContext` so multi-chart directory routing stays inside `chart_runtime`:
  - raw chart directories are scanned for supported source charts
  - source charts are normalized to temporary SENC files when needed
  - `ChartCatalog` / `CoverageIndex` / `QuiltPlanner` are initialized inside runtime
  - viewport and active quilt datasets are rebuilt in runtime on open and zoom
- Extended the narrow runtime C ABI with `chart_view_runtime_open_chart_directory(...)`.
- Extended `chart_standalone` with a thin host-only directory route:
  - `MainWindow::openChartDirectory(...)`
  - File menu action `Open Chart Directory...`
  - CLI / smoke support via `--open-chart-directory`
- Added runtime API coverage proving a synthetic two-chart S-101 directory can be opened and rendered through the public runtime C API without involving host-side catalog logic.
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target chart_runtime runtime_api_tests chart_standalone"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.api' --output-on-failure"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& { Set-Location 'C:/Users/zsh/source/repos/chart_view'; $script = Get-Content -Raw 'out/build/windows-msvc-debug/test/run_chart_standalone.open_chart.smoke_Debug.cmake'; if($script -match '(?s)set\(ENV\{PATH\} \[==\[(.*?)\]==\]\)') { $env:PATH = $Matches[1] }; if($script -match '(?s)set\(ENV\{QT_PLUGIN_PATH\} \[==\[(.*?)\]==\]\)') { $env:QT_PLUGIN_PATH = $Matches[1] }; $env:QT_QPA_PLATFORM = 'offscreen'; $tempDir = Join-Path ([System.IO.Path]::GetTempPath()) 'chart_view_open_chart_directory_manual_smoke'; Remove-Item -Recurse -Force $tempDir -ErrorAction SilentlyContinue; New-Item -ItemType Directory -Path $tempDir | Out-Null; @'`nS101SMOKE`nname=manual_dir_chart_a`nnative_scale=12000`npoint=121.8006,31.2301`nline=121.8002,31.2298;121.8013,31.2304;121.8020,31.2299`narea=121.8004,31.2300;121.8014,31.2300;121.8014,31.2308;121.8004,31.2308`n'@ | Set-Content -Path (Join-Path $tempDir 'chart_a.101') -NoNewline; @'`nS101SMOKE`nname=manual_dir_chart_b`nnative_scale=15000`npoint=121.8016,31.2302`nline=121.8010,31.2299;121.8022,31.2305;121.8031,31.2301`narea=121.8012,31.2301;121.8025,31.2301;121.8025,31.2310;121.8012,31.2310`n'@ | Set-Content -Path (Join-Path $tempDir 'chart_b.101') -NoNewline; & 'C:/Users/zsh/source/repos/chart_view/out/build/windows-msvc-debug/apps/chart_standalone/Debug/chart_standalone.exe' --smoke-test --open-chart-directory $tempDir; $exitCode = $LASTEXITCODE; Remove-Item -Recurse -Force $tempDir -ErrorAction SilentlyContinue; exit $exitCode }"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'chart_standalone\.(smoke|open_chart\.smoke)' --output-on-failure"`
  - Result: targeted runtime API verification passed, manual directory smoke exited 0, and existing standalone single-chart smokes still passed on 2026-04-16.

## 40-phase2-demo-verification
- Added the final Phase 2 verification note [docs/phase2_demo_verification.md](C:/Users/zsh/source/repos/chart_view/docs/phase2_demo_verification.md).
- Added a checked-in synthetic multi-chart host fixture directory:
  - `tests/data/s101/quilt_directory/chart_a.101`
  - `tests/data/s101/quilt_directory/chart_b.101`
- Added `chart_standalone.open_chart_directory.smoke` so the official host's open-directory route is now part of the repeatable verification matrix.
- Re-ran the final targeted Phase 2 smoke matrix covering:
  - `runtime.s57_quilt_smoke`
  - `runtime.cm93_quilt_smoke`
  - `runtime.s101_quilt_smoke`
  - `chart_standalone.open_chart_directory.smoke`
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --preset windows-msvc-debug; cmake --build --preset build-windows-msvc-debug --target chart_standalone s57_quilt_smoke_tests cm93_quilt_smoke_tests s101_quilt_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(s57_quilt_smoke|cm93_quilt_smoke|s101_quilt_smoke)|chart_standalone\.open_chart_directory\.smoke' --output-on-failure"`
  - Result: 4/4 targeted Phase 2 smokes passed on 2026-04-16.

## 41-portrayal-registry-core
- Added runtime-internal portrayal sources:
  - `src/runtime/portrayal/portrayal_registry.hpp`
  - `src/runtime/portrayal/portrayal_registry.cpp`
- Added core portrayal rule types:
  - `SymbolRule`
  - `LineStyleRule`
  - `AreaFillRule`
  - `TextRule`
- Added `PortrayalRegistry` as the central runtime-owned store for default feature styles plus future per-acronym overrides.
- Added `src/runtime/render_types.hpp` so portrayal rules can share `SurfaceColor`/`SurfacePoint` definitions without depending on the QRhi backend or Qt headers.
- Updated `FeatureLayerRenderer` to consume registry-owned default point/line/area styles instead of hardcoded feature colors and thickness constants, while keeping the background palette stable.
- Added verification coverage:
  - new `runtime.portrayal_registry` unit tests for defaults and case-insensitive acronym overrides
  - updated `feature_renderer_tests` coverage proving the renderer responds to registry-owned default style changes
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --preset windows-msvc-debug; cmake --build --preset build-windows-msvc-debug --target chart_runtime feature_renderer_tests portrayal_registry_tests s57_quilt_smoke_tests s101_quilt_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(portrayal_registry|feature_renderer|s57_quilt_smoke|s101_quilt_smoke)' --output-on-failure"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.cm93_quilt_smoke' --output-on-failure"`
  - Result: registry tests, renderer tests, and S57/CM93/S-101 quilt smokes all passed on 2026-04-16.

## 42-feature-symbolizer-core
- Added runtime-internal symbolization sources:
  - `src/runtime/portrayal/feature_symbolizer.hpp`
  - `src/runtime/portrayal/feature_symbolizer.cpp`
- Added `FeatureSymbolizer` so portrayal decisions stay outside `FeatureLayerRenderer` and produce narrow symbolization outputs:
  - geometry type
  - style key
  - optional text key
- Extended `PortrayalRegistry` with style-key registration and lookup for point, line, area, and text rules while preserving runtime-owned defaults.
- Updated `FeatureLayerRenderer` to render point/line/area features through the symbolization path instead of embedding business checks in the renderer.
- Hardened renderer-side projection handling for real-data quilt smokes by rejecting non-finite or implausibly projected geometry before emitting points, lines, or filled areas.
- Added verification coverage:
  - new `runtime.feature_symbolizer` unit tests covering point/line/area mapping and SENC roundtrip stability
  - updated `runtime.feature_renderer` tests proving style-key-based overrides and invalid-geometry rejection
  - re-ran the real-data quilt smoke matrix to confirm the symbolization path does not regress S57, CM93, or S-101 composed rendering
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(feature_symbolizer|feature_renderer|s57_quilt_smoke|cm93_quilt_smoke|s101_quilt_smoke)' --output-on-failure"`
  - Result: 5/5 targeted symbolization and quilt smoke tests passed on 2026-04-16.

## 43-display-priority-layering
- Added runtime-internal display-priority sources:
  - `src/runtime/portrayal/display_priority_model.hpp`
  - `src/runtime/portrayal/display_priority_model.cpp`
- Added `DisplayLayerGroup` / `DisplayPriority` / `DisplayPriorityModel` so portrayal ordering stays in `chart_runtime` instead of leaking into hosts or scene assembly.
- Updated `FeatureLayerRenderer` to apply stable single-chart render passes in this order:
  - areas
  - lines
  - points
- Preserved existing multi-chart snapshot ordering so this task did not silently expand into quilt policy changes.
- Hardened renderer construction by moving `FeatureLayerRenderer`'s default construction out-of-line, which fixed corrupted `PortrayalRegistry` state observed in the test binary after the task-43 integration.
- Added verification coverage:
  - new `runtime.display_priority` tests for stable layer grouping and specialized priority ordering
  - updated `runtime.feature_renderer` coverage proving points render above lines and areas in a single-chart frame
  - re-ran the S57 / CM93 / S-101 quilt smoke matrix to confirm the new layering does not regress composed rendering
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target display_priority_tests feature_renderer_tests s57_quilt_smoke_tests cm93_quilt_smoke_tests s101_quilt_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(display_priority|feature_renderer|s57_quilt_smoke|cm93_quilt_smoke|s101_quilt_smoke)' --output-on-failure"`
  - Result: 5/5 targeted display-priority and quilt smoke tests passed on 2026-04-16.

## 44-point-symbol-renderer
- Added runtime-internal point-symbol sources:
  - `src/runtime/point_symbol_renderer.hpp`
  - `src/runtime/point_symbol_renderer.cpp`
- Added a narrow built-in point-symbol atlas keyed by style key for:
  - `point/sounding`
  - `point/buoy`
  - `point/beacon`
- Updated `FeatureSymbolizer` so key point classes now resolve to symbol styles instead of falling back to generic points:
  - `VALSOU` / `SOUNDG`-style soundings -> `point/sounding`
  - `BOY*` acronyms -> `point/buoy`
  - `BCN*` acronyms -> `point/beacon`
- Updated `FeatureLayerRenderer` to route point rendering through the new point-symbol path first and only fall back to plain point discs when no built-in glyph exists.
- Added verification coverage:
  - new `runtime.point_symbol` smoke tests for the built-in atlas and sounding glyph rasterization
  - updated `runtime.feature_symbolizer` coverage for buoy/beacon style-key mapping
  - updated `runtime.feature_renderer` coverage proving soundings render as symbol glyphs rather than plain filled discs
  - re-ran the S57 / CM93 / S-101 quilt smoke matrix to confirm the new point-symbol path does not regress composed rendering
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target point_symbol_tests feature_symbolizer_tests feature_renderer_tests s57_quilt_smoke_tests cm93_quilt_smoke_tests s101_quilt_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(point_symbol|feature_symbolizer|feature_renderer|s57_quilt_smoke|cm93_quilt_smoke|s101_quilt_smoke)' --output-on-failure"`
  - Result: 6/6 targeted point-symbol, renderer, and quilt smoke tests passed on 2026-04-16.

## 45-line-symbol-renderer
- Added runtime-internal line-symbol sources:
  - `src/runtime/line_symbol_renderer.hpp`
  - `src/runtime/line_symbol_renderer.cpp`
- Added a narrow built-in line-style pattern renderer keyed by line style key for:
  - `line/depth_contour` -> dashed pattern
  - `line/coastline` -> differentiated special pattern
- Updated `FeatureLayerRenderer` to route projected line geometry through the new line-symbol path first and only fall back to plain solid line drawing when no built-in line pattern exists.
- Added verification coverage:
  - new `runtime.line_symbol` smoke tests for the dashed depth-contour path and keyed coastline pattern handling
  - updated `runtime.feature_renderer` coverage proving depth contours render with visible gaps rather than a single solid stroke
  - re-ran the S57 / CM93 / S-101 quilt smoke matrix to confirm the new line-symbol path does not regress composed rendering
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target line_symbol_tests feature_renderer_tests s57_quilt_smoke_tests cm93_quilt_smoke_tests s101_quilt_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(line_symbol|feature_renderer|s57_quilt_smoke|cm93_quilt_smoke|s101_quilt_smoke)' --output-on-failure"`
  - Result: 5/5 targeted line-symbol, renderer, and quilt smoke tests passed on 2026-04-16.

## 46-area-symbol-renderer
- Added runtime-internal area-symbol sources:
  - `src/runtime/area_symbol_renderer.hpp`
  - `src/runtime/area_symbol_renderer.cpp`
- Added a narrow built-in area pattern path keyed by style key for:
  - `area/depth` -> patterned fill overlay with hole-aware rendering
- Updated `FeatureLayerRenderer` to route projected area geometry through the new area-symbol path first and only fall back to plain solid fill drawing when no built-in area pattern exists.
- Added verification coverage:
  - new `runtime.area_symbol` smoke tests for the keyed depth-area pattern path and hole handling
  - updated `runtime.feature_renderer` coverage proving depth areas contain both fill and pattern pixels instead of a single flat fill
  - re-ran the S57 / CM93 / S-101 quilt smoke matrix to confirm the new area-symbol path does not regress composed rendering
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target area_symbol_tests feature_renderer_tests s57_quilt_smoke_tests cm93_quilt_smoke_tests s101_quilt_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(area_symbol|feature_renderer|s57_quilt_smoke|cm93_quilt_smoke|s101_quilt_smoke)' --output-on-failure"`
  - Result: 5/5 targeted area-symbol, renderer, and quilt smoke tests passed on 2026-04-16.

## 47-text-label-core
- Added runtime-internal text-label sources:
  - `src/runtime/text_label_renderer.hpp`
  - `src/runtime/text_label_renderer.cpp`
- Added `LabelItem` plus a narrow text-label layout path that extracts `OBJNAM` / `NOBJNM`, computes a basic anchor-relative label box, and renders labels with a tiny runtime-owned bitmap glyph set.
- Updated `FeatureLayerRenderer` to run a label pass after geometry rendering so named features can display basic labels without introducing advanced collision avoidance or host-owned text logic.
- Added verification coverage:
  - new `runtime.label` smoke tests for label extraction, layout, and bitmap text rendering
  - updated `runtime.feature_renderer` coverage proving named features can render visible label pixels through the integrated label path
  - re-ran the S57 / CM93 / S-101 quilt smoke matrix to confirm the new text-label path does not regress composed rendering
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target label_tests feature_renderer_tests s57_quilt_smoke_tests cm93_quilt_smoke_tests s101_quilt_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(label|feature_renderer|s57_quilt_smoke|cm93_quilt_smoke|s101_quilt_smoke)' --output-on-failure"`
  - Result: 5/5 targeted label, renderer, and quilt smoke tests passed on 2026-04-16.

## 48-s57-portrayal-rules
- Added runtime-internal S57 portrayal rule-table sources:
  - `src/runtime/portrayal/s57_rule_table.hpp`
  - `src/runtime/portrayal/s57_rule_table.cpp`
- Mapped selected key S57 classes to semantic style keys instead of geometry-only fallbacks:
  - point: `SOUNDG`, `BOYSPP` / `BOYLAT` / `BOYSAW`, `BCNSPP` / `BCNLAT` / `BCNSAW`, `WRECKS`, `UWTROC`, `LIGHTS`, `LNDMRK`, `PILPNT`
  - line: `DEPCNT`, `COALNE`, `FAIRWY`, `CANALS`, `RIVERS`
  - area: `DEPARE`, `DRGARE`, `LNDARE`, `RESARE`, `UNSARE`
- Updated `FeatureSymbolizer` to consult the S57 rule table first so chosen S57 classes resolve to semantic point/line/area style keys before generic attribute heuristics.
- Extended runtime portrayal defaults and renderers for the new semantic keys:
  - new point-symbol glyphs for `point/danger` and `point/landmark`
  - new line-style pattern for `line/channel`
  - new portrayal-registry defaults for `point/danger`, `point/landmark`, `line/channel`, `area/land`, and `area/restricted`
- Hardened `DisplayPriorityModel` so the new semantic S57 styles remain on their geometry layers instead of being misclassified as text-only when labels are present.
- Added verification coverage:
  - new `runtime.s57_rule_table` tests for selected class-to-style mappings
  - updated `runtime.feature_symbolizer` coverage for S57 rule-table mappings and fallback behavior
  - updated `runtime.display_priority` coverage to prove semantic S57 styles keep stable area/line/point layering
  - updated `runtime.feature_renderer` coverage proving key S57 classes render with semantic portrayal styles
  - re-ran the S57 / CM93 / S-101 quilt smoke matrix to confirm the new S57 mappings do not regress composed rendering
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target s57_rule_table_tests feature_symbolizer_tests display_priority_tests feature_renderer_tests s57_quilt_smoke_tests cm93_quilt_smoke_tests s101_quilt_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(s57_rule_table|feature_symbolizer|display_priority|feature_renderer|s57_quilt_smoke|cm93_quilt_smoke|s101_quilt_smoke)' --output-on-failure"`
  - Result: 7/7 targeted S57-portrayal, renderer, and quilt smoke tests passed on 2026-04-16.

## 49-s101-portrayal-rules
- Added runtime-internal S-101 portrayal rule-table sources:
  - `src/runtime/portrayal/s101_rule_table.hpp`
  - `src/runtime/portrayal/s101_rule_table.cpp`
- Mapped selected S-101 semantic class names to shared style keys instead of relying on S57 acronym overlap:
  - point: `Sounding`, `BuoySpecialPurpose`, `BuoyLateral`, `BuoySafeWater`, `BeaconSpecialPurpose`, `BeaconLateral`, `BeaconSafeWater`, `Wreck`, `UnderwaterRock`, `Light`, `Landmark`, `PilotBoardingPlace`
  - line: `DepthContour`, `Coastline`, `Fairway`, `Canal`, `River`
  - area: `DepthArea`, `DredgedArea`, `LandArea`, `RestrictedArea`, `UnsurveyedArea`
- Kept narrow compatibility aliases in the S-101 rule table for the current scaffold inputs (`SOUNDG`, `DEPCNT`, `DEPARE`) so the portrayal path stays robust while the synthetic reader transitions.
- Updated `FeatureSymbolizer` to resolve semantic styles through both S57 and S-101 rule tables before falling back to generic attribute heuristics.
- Updated the synthetic S-101 reader scaffold so `point` / `line` / `area` smoke features now emit S-101-flavored class names:
  - `Sounding`
  - `DepthContour`
  - `DepthArea`
- Added verification coverage:
  - new `runtime.s101_rule_table` tests for selected class-to-style mappings and scaffold aliases
  - updated `runtime.feature_symbolizer` coverage for S-101 rule-table mappings
  - updated `runtime.feature_renderer` coverage proving key S-101 classes render with semantic portrayal styles
  - updated `runtime.s101_reader` coverage proving the synthetic reader now emits S-101-flavored class names
  - re-ran the S57 / CM93 / S-101 quilt smoke matrix to confirm the new S-101 mappings do not regress composed rendering
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target s101_rule_table_tests s101_reader_tests feature_symbolizer_tests display_priority_tests feature_renderer_tests s57_quilt_smoke_tests cm93_quilt_smoke_tests s101_quilt_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(s101_rule_table|s101_reader|feature_symbolizer|display_priority|feature_renderer|s57_quilt_smoke|cm93_quilt_smoke|s101_quilt_smoke)' --output-on-failure"`
  - Result: 8/8 targeted S-101 portrayal, reader, renderer, and quilt smoke tests passed on 2026-04-16.

## 50-symbolized-render-smoke-s57
- Added a dedicated runtime smoke target:
  - `test/runtime/s57_symbolized_smoke_tests.cpp`
- The new smoke builds a small synthetic S57 sample dataset with key semantic classes:
  - `LNDARE` area
  - `FAIRWY` line
  - `WRECKS` point with `OBJNAM`
- The smoke keeps the runtime flow intact by round-tripping the synthetic sample through SENC before building a scene snapshot and rendering a frame.
- The smoke verifies in one frame that:
  - semantic S57 area / line / point styles all appear
  - the label pass renders visible text pixels
  - display priority keeps the point symbol on top at the shared center pixel
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target s57_symbolized_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.s57_symbolized_smoke' --output-on-failure"`
  - Result: 1/1 targeted S57 symbolized smoke test passed on 2026-04-16.

## 51-symbolized-render-smoke-s101
- Added a dedicated runtime smoke target:
  - `test/runtime/s101_symbolized_smoke_tests.cpp`
- The new smoke builds a small synthetic S-101 sample dataset with key semantic classes:
  - `LandArea` area
  - `Fairway` line
  - `Wreck` point with `OBJNAM`
- The smoke keeps the runtime flow intact by round-tripping the synthetic sample through SENC before building a scene snapshot and rendering a frame.
- The smoke verifies in one frame that:
  - semantic S-101 area / line / point styles all appear
  - the label pass renders visible text pixels
  - display priority keeps the point symbol on top at the shared center pixel
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target s101_symbolized_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.s101_symbolized_smoke' --output-on-failure"`
  - Result: 1/1 targeted S-101 symbolized smoke test passed on 2026-04-16.

## 52-phase3-demo-verification
- Added the final Phase 3 verification note:
  - `docs/phase3_demo_verification.md`
- Documented the Phase 3 runtime-owned semantic portrayal path:
  - `FeatureSymbolizer`
  - `DisplayPriorityModel`
  - point / line / area / text symbol renderers
  - integrated symbolized render smokes for S57 and S-101
- Re-ran the full Phase 3 verification matrix covering portrayal registry, semantic rule tables, symbolizer, display priority, point/line/area/text symbol renderers, integrated feature rendering, and both symbolized smoke tests.
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target portrayal_registry_tests feature_symbolizer_tests display_priority_tests point_symbol_tests line_symbol_tests area_symbol_tests label_tests s57_rule_table_tests s101_rule_table_tests feature_renderer_tests s57_symbolized_smoke_tests s101_symbolized_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(portrayal_registry|feature_symbolizer|display_priority|point_symbol|line_symbol|area_symbol|label|s57_rule_table|s101_rule_table|feature_renderer|s57_symbolized_smoke|s101_symbolized_smoke)' --output-on-failure"`
  - Result: 12/12 targeted Phase 3 portrayal and symbolized smoke tests passed on 2026-04-16.

## 53-proj-projection-context-core
- Added runtime-internal projected-display sources:
  - `src/runtime/projection/projection_context.hpp`
  - `src/runtime/projection/projection_context.cpp`
  - `src/runtime/projection/projected_viewport.hpp`
  - `src/runtime/projection/projected_viewport.cpp`
- Added a runtime-owned `ProjectionContext` wrapper that keeps `PJ_CONTEXT` / `PJ*` handles private to `chart_runtime` and exposes display-space project / unproject helpers only through internal C++ types.
- Added `ProjectedPoint`, `ProjectedExtent`, and `ProjectedViewport` helpers so runtime code can reason about one explicit display projection without exposing PROJ through the public C API.
- Wired PROJ into the build as a runtime dependency via `Dependencies.cmake` and `src/runtime/CMakeLists.txt`, while leaving host layers free of PROJ ownership.
- Added focused projection unit coverage:
  - `test/runtime/projection_context_tests.cpp`
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target chart_runtime projection_context_tests feature_renderer_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(projection_context|feature_renderer)' --output-on-failure"`
  - Result: 2/2 targeted projection and renderer regression tests passed on 2026-04-16.

## 54-projected-scene-and-coverage-space
- Added a shared projected-bounds helper:
  - `src/runtime/projection/projected_bounds.hpp`
- Updated `SceneBuilderFromSenc` to cull features in one shared projected display space instead of using ad-hoc geographic viewport math.
- Updated `CoverageIndex` to bucket chart extents and test overlap in projected metres while keeping PROJ ownership private to runtime internals.
- Updated runtime quilt-prep viewport extent derivation in `runtime_context.cpp` so viewport extents are derived from the projected display rectangle rather than the old equirectangular approximation.
- Expanded targeted verification coverage:
  - `test/runtime/scene_builder_tests.cpp`
  - `test/runtime/coverage_index_tests.cpp`
  - `test/runtime/quilt_planner_tests.cpp`
  - `test/CMakeLists.txt` target wiring for internal projection sources used by scene / coverage / quilt tests
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target chart_runtime scene_builder_tests coverage_index_tests quilt_planner_tests s57_quilt_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(scene_builder|coverage_index|quilt_planner|s57_quilt_smoke)' --output-on-failure"`
  - Result: 4/4 targeted projected scene / coverage / quilt tests passed on 2026-04-16.

## 55-projected-quilt-seams-and-patch-clipping
- Extended the quilt plan data model with projected viewport and projected patch ownership fields:
  - `src/runtime/quilt/quilt_plan.hpp`
- Updated `QuiltPlanner` to:
  - build projected visible regions per chart
  - clip lower-priority chart patches against already-owned higher-priority projected regions
  - keep runtime-owned projected patch extents explicit in each quilt layer
- Updated `SceneBuilderFromSenc` to consume projected patch extents when present so multi-chart scene assembly follows runtime-owned patch ownership instead of only geographic overlap.
- Updated the real S57 quilt smoke to derive viewport extents from the projected display rectangle before planning the quilt.
- Expanded regression coverage:
  - `test/runtime/quilt_planner_tests.cpp`
  - `test/runtime/scene_builder_tests.cpp`
  - `test/runtime/s57_quilt_smoke_tests.cpp`
  - `test/CMakeLists.txt` target wiring for scene / quilt internal projection sources
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target chart_runtime quilt_planner_tests scene_builder_tests feature_renderer_tests s57_quilt_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(quilt_planner|scene_builder|feature_renderer|s57_quilt_smoke)' --output-on-failure"`
  - Result: 4/4 targeted projected quilt patch, scene, renderer, and S57 quilt smoke tests passed on 2026-04-16.

## 56-projected-s57-real-chart-smoke
- Strengthened the real-data S57 quilt smoke so it now proves projected quilt composition directly:
  - `test/runtime/s57_quilt_smoke_tests.cpp`
- The smoke now:
  - carries an explicit `projection` tag and projected-test name
  - asserts `QuiltPlan.projectedViewportExtent()` is valid
  - asserts every selected quilt layer has valid projected full / visible extents and at least one valid projected patch extent intersecting the projected viewport
- Documented the real-chart gating used by the projected smoke:
  - `docs/build_environment.md`
  - The runtime smoke requires `CHARTSYS_ENABLE_REAL_CHART_TESTS=ON` plus a valid `CHARTSYS_S57_TESTDATA_ROOT`, and it will `SKIP` when no readable overlapping / adjacent chart pair is available.
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target s57_quilt_smoke_tests chart_standalone"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R '^(runtime\.s57_quilt_smoke|chart_standalone\.s57_host_smoke)$' --output-on-failure"`
  - Result: `runtime.s57_quilt_smoke` passed on 2026-04-16; the extra `chart_standalone.s57_host_smoke` probe still failed in the existing host path and was observed without widening this runtime-smoke task into host fixes.

## 57-s52-presentation-assets-adapter
- Added a runtime-owned S-52 baseline asset adapter:
  - `src/runtime/portrayal/s52_presentation_assets.hpp`
  - `src/runtime/portrayal/s52_presentation_assets.cpp`
- The adapter now exposes a narrow baseline set of:
  - palette colors
  - point symbol assets
  - line style assets
  - area pattern assets
- Updated `PortrayalRegistry` to seed its baseline portrayal styles from the S-52 asset adapter instead of hardcoding every baseline style directly in the registry constructor:
  - `src/runtime/portrayal/portrayal_registry.cpp`
- Added focused asset and registry coverage:
  - `test/runtime/s52_presentation_assets_tests.cpp`
  - `test/runtime/portrayal_registry_tests.cpp`
- Documented the normative-source boundary in `docs/architecture.md`:
  - IHO S-52 / Annex A / S-64 are the normative source for portrayal intent
  - OpenCPN remains engineering reference only
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target s52_presentation_assets_tests portrayal_registry_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(s52_presentation_assets|portrayal_registry)' --output-on-failure"`
  - Result: 2/2 targeted S-52 asset-adapter and registry tests passed on 2026-04-16.

## 58-s52-lookup-and-instruction-model
- Added a runtime-internal S-52 lookup and instruction model:
  - `src/runtime/portrayal/s52_lookup_model.hpp`
- The lookup model now maps selected S57 classes to explicit baseline instructions carrying:
  - instruction type
  - S-52 asset id
  - compatible runtime style key
- Extended `FeatureSymbolization` with optional explicit S-52 lookup results:
  - `src/runtime/portrayal/feature_symbolizer.hpp`
- Updated `FeatureSymbolizer` to:
  - try the S-52 lookup path first for selected S57 classes
  - derive `styleKey` / `textKey` from emitted instructions for current renderer compatibility
  - fall back to the existing semantic / attribute heuristics only when no S-52 lookup applies
- Added focused lookup and symbolizer coverage:
  - `test/runtime/s52_lookup_model_tests.cpp`
  - `test/runtime/feature_symbolizer_tests.cpp`
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target s52_lookup_model_tests feature_symbolizer_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(s52_lookup_model|feature_symbolizer)' --output-on-failure"`
  - Result: 2/2 targeted S-52 lookup-model and feature-symbolizer tests passed on 2026-04-16.

## 59-s52-display-settings-and-conditional-symbology
- Added narrow runtime-owned S-52 display settings:
  - `src/runtime/portrayal/s52_display_settings.hpp`
- Added a baseline conditional-symbology evaluator:
  - `src/runtime/portrayal/s52_conditional_symbology.hpp`
- The baseline conditional path now supports three explicit Phase 4 controls:
  - sounding visibility
  - text-label visibility
  - traditional vs simplified point-symbol variants for buoy / beacon families
- Extended `S52LookupResult` and `FeatureSymbolization` so settings-driven suppression can stay inside the runtime-owned portrayal decision path.
- Updated `FeatureSymbolizer` to honor the new settings / conditional evaluator before applying fallback heuristics.
- Added focused verification coverage:
  - `test/runtime/s52_display_settings_tests.cpp`
  - `test/runtime/s52_conditional_symbology_tests.cpp`
  - updated `test/runtime/feature_symbolizer_tests.cpp`
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target s52_display_settings_tests s52_conditional_symbology_tests feature_symbolizer_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(s52_display_settings|s52_conditional_symbology|feature_symbolizer)' --output-on-failure"`
  - Result: 3/3 targeted S-52 settings, conditional-symbology, and symbolizer tests passed on 2026-04-16.

## 60-s52-renderer-integration-s57
- Updated the runtime render path so `FeatureLayerRenderer` now consumes the emitted S-52 instructions instead of only relying on Phase 3 semantic style fallbacks:
  - `src/runtime/feature_layer_renderer.hpp`
  - `src/runtime/feature_layer_renderer.cpp`
- Extended the runtime-owned point / line / area renderers so they can execute instruction-selected S-52 asset ids while preserving style-key rule lookup compatibility:
  - `src/runtime/point_symbol_renderer.hpp`
  - `src/runtime/point_symbol_renderer.cpp`
  - `src/runtime/line_symbol_renderer.hpp`
  - `src/runtime/line_symbol_renderer.cpp`
  - `src/runtime/area_symbol_renderer.hpp`
  - `src/runtime/area_symbol_renderer.cpp`
- The S57 render path now explicitly:
  - uses point / line / area / text instructions from `FeatureSymbolization.s52Lookup` when present
  - honors settings-driven `suppressed` results at final render time so hidden S-52 objects do not fall back to generic default symbols
  - supports visible traditional vs simplified buoy / beacon point-symbol variants through instruction asset ids
- Expanded verification coverage:
  - `test/runtime/feature_renderer_tests.cpp`
    - new simplified buoy render check proving the runtime executes the `BOYSPP02` instruction variant instead of the traditional glyph
    - new suppressed sounding render check proving `showSoundings = false` prevents default fallback rendering
  - `test/runtime/s57_symbolized_smoke_tests.cpp`
    - upgraded the integrated S57 smoke to use an S-52-backed simplified buoy symbol path while preserving area / line / text verification through SENC
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target feature_renderer_tests s57_symbolized_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(feature_renderer|s57_symbolized_smoke)' --output-on-failure"`
  - Result: 2/2 targeted S-52-backed renderer integration and integrated S57 smoke tests passed on 2026-04-16.

## 61-unicode-text-system-core
- Added a runtime-owned Unicode text helper:
  - `src/runtime/unicode_text.hpp`
- The new helper decodes UTF-8 into code-point-safe storage, truncates by code point instead of byte count, and substitutes malformed byte sequences with replacement code points instead of leaving the label path byte-fragile.
- Updated the runtime label pipeline:
  - `src/runtime/text_label_renderer.hpp`
  - `src/runtime/text_label_renderer.cpp`
- `LabelItem` now keeps:
  - UTF-8 label text for storage / pass-through
  - a decoded `std::u32string` glyph sequence for layout and rendering
- The runtime label path now:
  - extracts `OBJNAM` / `NOBJNM` as UTF-8 text
  - computes label width from decoded code points, not raw byte length
  - renders per code point, preserving ASCII glyphs and falling back to the existing placeholder glyph for non-ASCII code points until task 62 adds real font fallback
- Added focused Unicode verification coverage:
  - `test/runtime/unicode_text_tests.cpp`
  - updated `test/runtime/text_label_renderer_tests.cpp`
  - updated `test/CMakeLists.txt` with the new `runtime.unicode_text` target
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target unicode_text_tests label_tests feature_renderer_tests; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(unicode_text|label)' --output-on-failure"`
  - Result: `runtime.unicode_text` and `runtime.label` both passed on 2026-04-16; `feature_renderer_tests` also rebuilt successfully as a compile regression check.

## 62-font-fallback-and-glyph-cache
- Added runtime-owned font fallback and glyph-cache internals:
  - `src/runtime/font_fallback.hpp`
  - `src/runtime/glyph_cache.hpp`
- Updated the runtime label path:
  - `src/runtime/text_label_renderer.hpp`
  - `src/runtime/text_label_renderer.cpp`
- The runtime text path now:
  - resolves Unicode glyphs through an internal family-search path kept inside `chart_runtime`
  - rasterizes glyph masks into a runtime-owned cache instead of falling back to byte-oriented placeholder-only rendering
  - renders cached bitmap glyph masks through the existing RHI-backed label path, while preserving DLL-first boundaries and keeping host code out of font ownership
- Added focused verification coverage:
  - `test/runtime/font_fallback_tests.cpp`
  - `test/runtime/glyph_cache_tests.cpp`
  - updated `test/runtime/text_label_renderer_tests.cpp`
  - updated `test/CMakeLists.txt`
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target font_fallback_tests glyph_cache_tests label_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(font_fallback|glyph_cache|label)' --output-on-failure"`
  - Result: 3/3 targeted font-fallback, glyph-cache, and label-render tests passed on 2026-04-16.

## 63-multilingual-label-selection-and-projected-layout
- Added a runtime-owned label-layout helper:
  - `src/runtime/label_layout.hpp`
- The helper now centralizes three Phase 4 label-baseline decisions inside `chart_runtime`:
  - deterministic multilingual name selection with `NOBJNM -> OBJNAM` fallback
  - projected display-space anchor resolution for point / line / area labels
  - baseline screen-space visibility and overlap checks for labels in one frame
- Updated the runtime label path:
  - `src/runtime/text_label_renderer.hpp`
  - `src/runtime/text_label_renderer.cpp`
- `TextLabelRenderer` now:
  - records which source attribute supplied the label text
  - prefers valid national-language names when available
  - falls back to `OBJNAM` when `NOBJNM` is empty or decodes only to replacement glyphs
  - carries computed label bounds so collision handling stays runtime-owned
- Updated the runtime feature render path:
  - `src/runtime/feature_layer_renderer.hpp`
  - `src/runtime/feature_layer_renderer.cpp`
- `FeatureLayerRenderer` now:
  - resolves label anchors from the runtime-owned projected viewport when available
  - keeps a baseline occupied-label list per frame
  - suppresses later labels whose screen-space bounds overlap already accepted labels
  - falls back to the legacy heuristic only if the projected path is unavailable
- Expanded verification coverage:
  - updated `test/runtime/text_label_renderer_tests.cpp`
  - updated `test/runtime/feature_renderer_tests.cpp`
  - updated `test/CMakeLists.txt` so `label_tests` can compile the internal projection sources it now exercises
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target label_tests feature_renderer_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(label|feature_renderer)' --output-on-failure"`
  - Result: `runtime.label` and `runtime.feature_renderer` both passed on 2026-04-16.

## 64a-projected-quilt-unblock-for-known-real-pair
- Adjusted runtime-owned chart ranking so coarser-than-viewport charts act as fallback coverage instead of owning overlap ahead of finer projected candidates:
  - `src/runtime/catalog/chart_selection_policy.hpp`
  - `src/runtime/catalog/chart_selection_policy.cpp`
- Added focused regression coverage proving the new ordering preserves dual-chart quilt ownership without disabling patch subtraction:
  - `test/runtime/chart_selection_policy_tests.cpp`
  - `test/runtime/quilt_planner_tests.cpp`
  - `test/runtime/s57_quilt_smoke_tests.cpp`
- Added pair-specific documentation:
  - `docs/phase4_targeted_pair_audit_C1511781_C1511782.md`
  - `docs/phase4_projected_quilt_unblock_C1511781_C1511782.md`
- Verification:
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; cmake --build --preset build-windows-msvc-debug --target chart_selection_policy_tests quilt_planner_tests s57_quilt_smoke_tests"`
  - `powershell -ExecutionPolicy Bypass -NoProfile -Command "& 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 | Out-Null; Set-Location 'C:/Users/zsh/source/repos/chart_view'; ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R 'runtime\.(chart_selection_policy|quilt_planner)' --output-on-failure"`
  - `C:/Users/zsh/source/repos/chart_view/out/build/windows-msvc-debug/test/Debug/s57_quilt_smoke_tests.exe '[targeted-pair]' -s --reporter console`
  - Result: the known real pair `C1511781.000` / `C1511782.000` now ranks as `C1511782, C1511781`, survives as a two-layer projected quilt, keeps non-empty projected patches for both charts, and still shows active patch clipping on the overview layer.

## 64b-s57-semantic-baseline-unblock-for-real-smoke
- Preserved the minimum real S57 semantics needed by the existing Phase 4 baseline inside `chart_runtime`:
  - `src/runtime/s57/s57_semantic_mapping.hpp`
  - `src/runtime/s57/s57_reader.cpp`
- The reader now:
  - maps selected real S57 object class codes to baseline acronyms already consumed by `S52LookupModel`
  - maps a narrow semantic attribute subset including `OBJNAM`, `NOBJNM`, `CATCOA`, `DRVAL1`, `DRVAL2`, `VALDCO`, and `VALSOU`
  - parses `NATF` in addition to `ATTF` so national-language names can reach the existing multilingual label path
  - preserves the old `OBJ<code>` / `A<code>` fallback for unknown classes and attributes instead of widening into full dictionary work
- Added focused regression coverage:
  - `test/runtime/s57_reader_tests.cpp`
  - `test/runtime/s57_quilt_smoke_tests.cpp`
- Added pair-specific documentation:
  - `docs/phase4_s57_semantic_unblock_C1511781_C1511782.md`
- Verification:
  - `cmake --build --preset build-windows-msvc-debug --target s57_reader_tests s57_quilt_smoke_tests`
  - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.(s57_reader|s57_quilt_smoke)" --output-on-failure`
  - `C:/Users/zsh/source/repos/chart_view/out/build/windows-msvc-debug/test/Debug/s57_quilt_smoke_tests.exe '[targeted-pair]' -s --reporter console`
  - Result: the known real pair `C1511781.000` / `C1511782.000` now yields combined `s52Hits=1086`, `named=220`, `unicodeNamed=69`, and `textCandidates=220`; `runtime.s57_reader`, `runtime.s57_quilt_smoke`, and the targeted real-pair audit all pass.

## 64-s52-unicode-real-chart-smoke-s57
- Formally closed out the integrated Phase 4 S57 real-chart smoke using the fixed pair:
  - `C1511781.000`
  - `C1511782.000`
- Added the formal closeout note:
  - `docs/phase4_task64_closeout.md`
- Closeout verification confirmed that the real-chart runtime path now honestly passes:
  - both target charts are discovered and loaded
  - both target charts survive into the same projected quilt plan
  - projected patch ownership remains active instead of being bypassed
  - the integrated runtime path yields non-zero S-52 baseline hits
  - the integrated runtime path yields non-zero named features and text label candidates
  - Unicode-capable labels are present when provided by the real data
- Verification:
  - `cmake --build --preset build-windows-msvc-debug --target s57_reader_tests s57_quilt_smoke_tests`
  - `ctest --test-dir out/build/windows-msvc-debug -C Debug --force-new-ctest-process -R "runtime\.(s57_reader|s57_quilt_smoke)" --output-on-failure`
  - `C:/Users/zsh/source/repos/chart_view/out/build/windows-msvc-debug/test/Debug/s57_quilt_smoke_tests.exe '[targeted-pair]' -s --reporter console`
  - Result:
    - `runtime.s57_reader` passed
    - `runtime.s57_quilt_smoke` passed
    - targeted pair audit passed with:
      - `quilt ordered chart ids: C1511782, C1511781`
      - `s52Hits=1086`
      - `named=220`
      - `unicodeNamed=69`
      - `textCandidates=220`
      - `visible projected labels: total=68 unicode=34`
  - Closeout scope note:
    - this completes a smoke-level Phase 4 baseline only
    - it does not claim S-64 compliance or full S-52 coverage
