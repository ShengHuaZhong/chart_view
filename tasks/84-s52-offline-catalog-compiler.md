# 84 - S-52 offline catalog compiler

## Objective
Compile the official Phase 6 Annex A asset sources into deterministic runtime-friendly catalog and IR artifacts.

## Phase
- Phase 6

## Layer
- tooling
- runtime portrayal internals

## Depends on
- 83

## In scope
- Add offline compiler tooling
- Emit deterministic binary or packed catalog artifacts
- Switch runtime loading to prefer compiled official catalogs

## Out of scope
- No fuller S57 coverage expansion yet
- No CSP VM yet
- No host changes

## Inputs
- task 83 vendored official assets
- existing Phase 5 compiled-catalog structures

## Required changes
- Compile official symbols, colours, lookup rows, display metadata, and CSP metadata into runtime-owned artifacts
- Preserve stable rule ids and provenance metadata
- Demote the old built-in catalog to bootstrap/test fallback only

## Deliverables
- Offline compiler
- Deterministic compiled artifacts
- Runtime loader changes for the official compiled path

## Done when
The runtime can load a compiled official catalog as the primary portrayal source and the compiler output is deterministic across repeated runs.

## Verification
- Build the catalog-compiler target and related runtime catalog tests
- Run deterministic-compile and runtime-load coverage

## Notes
- From this task onward, the built-in subset must not remain the main verification path.
