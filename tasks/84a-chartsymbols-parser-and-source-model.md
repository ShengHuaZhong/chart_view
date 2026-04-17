# 84a - chartsymbols parser and source model

## Objective
Parse OpenCPN `chartsymbols.xml` and supporting bundle metadata into an expanded `S52SourceCatalog` suitable for Phase 6A compilation.

## Phase
- Phase 6A

## Layer
- runtime tooling
- portrayal catalog input model

## Depends on
- 83a

## In scope
- Add `chartsymbols.xml` parsing
- Extend `S52SourceCatalog` to carry richer lookup, symbol, line, pattern, text, and conditional metadata
- Add focused parser correctness tests

## Out of scope
- No runtime compiled-catalog loading yet
- No graphics engine execution changes
- No public ABI changes
