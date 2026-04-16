# 67 - S57 source model and update manifest

## Objective
Introduce an internal S57 source/domain layer that preserves source facts and update-manifest metadata before the runtime derives `FeatureChartDataset`.

## Phase
- Phase 5

## Layer
- runtime
- chart_data
- verification

## Depends on
- 66

## In scope
- Add an internal `S57SourceModel`
- Preserve source identity, source manifest, and update-manifest facts
- Refactor `S57Reader` to build source-model-first, then derive the existing dataset
- Add focused tests for source manifest and update-manifest scanning

## Out of scope
- No update application yet
- No SENC v2 yet
- No public C API changes
- No renderer / S-52 / host changes

## Inputs
- AGENTS.md
- docs/phase_roadmap.md
- docs/architecture.md
- src/runtime/s57/s57_reader.*
- src/runtime/senc/source_manifest.hpp

## Required changes
- Add `src/runtime/s57/s57_source_model.*`
- Extend `S57ReadResult` with source-model output
- Preserve FOID / FRID / FSPT / standard vs national attributes in the internal model
- Build source and update manifests from the base chart file

## Deliverables
- Internal source-model implementation
- Reader refactor with no public ABI expansion
- Unit coverage for source/update manifests

## Done when
The runtime can read a base `.000` chart into a stable `S57SourceModel`, populate source/update manifest metadata, and still produce the existing `FeatureChartDataset` path without breaking current S57 SENC smoke coverage.

## Verification
- Build `s57_reader_tests` and `s57_senc_smoke_tests`
- Run `runtime.s57_reader`
- Run `runtime.s57_senc_smoke`

## Notes
- Preserve Phase 4 behavior while creating the Phase 5 hook point.
- Do not silently start applying `.001+` updates in this task.
