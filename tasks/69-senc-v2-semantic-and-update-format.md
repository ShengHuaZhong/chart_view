# 69 - SENC v2 semantic and update format

## Objective
Add a Phase 5 SENC v2 that can persist richer S57 semantics plus applied-update state while keeping SENC v1 compatibility.

## Phase
- Phase 5

## Layer
- runtime
- senc
- verification

## Depends on
- 68

## In scope
- Define SENC v2 headers / sections
- Persist richer S57 semantic payloads and update manifests
- Keep existing SENC v1 readers/tests intact

## Out of scope
- No public ABI expansion for deep internals
- No full portrayal integration yet

## Inputs
- AGENTS.md
- docs/phase_roadmap.md
- tasks/68-s57-update-application-core.md

## Required changes
- Add v2 section model and reader/writer support
- Preserve SENC v1 compatibility
- Add roundtrip tests for update-manifest persistence

## Deliverables
- SENC v2 types, writer, reader, and tests

## Done when
The runtime can write and read SENC v2 with richer S57 semantics and applied-update metadata while existing v1 behavior remains available.

## Verification
- Build focused SENC v2 tests
- Run v1/v2 roundtrip coverage

## Notes
- Keep format ownership inside `chart_runtime`.
