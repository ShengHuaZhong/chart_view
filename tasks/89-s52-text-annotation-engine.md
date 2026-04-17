# 89 - S-52 text and annotation engine

## Objective
Promote the Phase 4/5 Unicode label baseline into a fuller S-52 text and annotation execution engine.

## Phase
- Phase 6

## Layer
- runtime portrayal
- runtime text

## Depends on
- 88

## In scope
- Official text instruction execution
- Priority-aware annotation drawing
- National/international name selection integration
- Placement and suppression behavior consistent with the Phase 6 portrayal path

## Out of scope
- No new text foundation rewrite
- No host text logic
- No mariner-settings completion yet

## Inputs
- official compiled text instructions
- existing Unicode-safe text, font fallback, and glyph cache

## Required changes
- Integrate official text instructions into the runtime label/render path
- Reuse the existing Unicode/fallback foundation
- Add focused annotation regression tests

## Deliverables
- Runtime-owned S-52 text/annotation engine
- Regression coverage for text instruction behavior

## Done when
Text and annotations are executed through official Phase 6 instruction data on top of the existing Unicode-safe text system.

## Verification
- Build text/annotation tests and affected real-chart or synthetic smoke targets
- Run the focused text regression suite

## Notes
- Do not reduce this work to `wchar_t` plumbing or move label logic into the host.
