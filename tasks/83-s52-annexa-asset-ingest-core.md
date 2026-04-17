# 83 - S-52 Annex A asset ingest core

## Objective
Introduce the repository-owned ingest path for official Annex A, Chart 1, and selected S-64 source assets, with provenance manifests and versioned input layout.

## Phase
- Phase 6

## Layer
- tooling
- docs
- asset manifests

## Depends on
- 82

## In scope
- Add a controlled vendored source tree for official assets
- Define provenance-manifest schema and repository layout
- Ingest the official Annex A source set needed for the compiler pipeline

## Out of scope
- No runtime rendering changes
- No catalog compiler output yet
- No public ABI changes

## Inputs
- official Annex A digital files
- DAI source
- Chart 1 assets
- selected S-64 reference inputs

## Required changes
- Add the vendored Phase 6 asset tree
- Add provenance manifests with version, clarification cut-off, file hashes, and source notes
- Add focused tooling/tests that verify the ingest tree is complete enough for the compiler stage

## Deliverables
- Vendored official asset tree
- Provenance manifests
- Ingest verification coverage

## Done when
The repository contains the Phase 6 official source assets and manifest metadata needed for the offline compiler, without yet changing runtime portrayal behavior.

## Verification
- Build the new asset-ingest verification target
- Run the focused ingest completeness test target

## Notes
- Keep raw official assets as compiler input only. Runtime must not consume them directly.
