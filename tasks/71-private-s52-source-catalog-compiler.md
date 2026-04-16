# 71 - Private S52 source catalog compiler

## Objective
Compile the private, runtime-owned S-52 source assets into a deterministic internal catalog format.

## Phase
- Phase 5

## Layer
- runtime
- portrayal
- verification

## Depends on
- 70

## In scope
- Add a private S-52 source catalog compiler
- Compile colors, symbols, lookup rows, and stable rule identifiers
- Keep the compiled catalog runtime-owned

## Out of scope
- No host consumption of raw S-52 assets
- No final renderer integration yet

## Inputs
- AGENTS.md
- docs/phase_roadmap.md
- tasks/70-complete-s57-dictionary-and-attribute-model.md

## Required changes
- Add compiler and compiled-catalog runtime types
- Add deterministic compiler tests

## Deliverables
- S-52 source compiler
- Compiled runtime catalog format
- Compiler verification coverage

## Done when
The repository can turn its private S-52 source assets into a deterministic runtime-owned compiled catalog without depending on OpenCPN runtime code.

## Verification
- Build compiled-catalog tests
- Run deterministic-output and lookup-row sanity tests

## Notes
- OpenCPN is an engineering reference only; do not copy its code.
