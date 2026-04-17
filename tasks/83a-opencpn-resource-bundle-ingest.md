# 83a - OpenCPN resource bundle ingest

## Objective
Vendor a fixed OpenCPN `data/s57data` snapshot into the repository as the approved Phase 6A engineering input bundle, with provenance metadata and completeness/hash verification.

## Phase
- Phase 6A

## Layer
- tooling
- docs
- asset manifests

## Depends on
- 82
- historical blocker record from 83

## In scope
- Add the vendored OpenCPN `s57data` snapshot
- Add provenance and license metadata
- Add focused completeness/hash verification coverage
- Add the Phase 6A task chain `83a-93a`

## Out of scope
- No runtime rendering changes
- No parser/compiler implementation yet
- No public ABI changes
- No `s52plib` integration

## Done when
The repository contains a pinned OpenCPN `s57data` snapshot and provenance metadata that are sufficient for the later Phase 6A parser/compiler tasks, without changing runtime portrayal behavior yet.

## Verification
- Build the new OpenCPN resource-bundle verification target
- Run the focused bundle completeness/hash test target
