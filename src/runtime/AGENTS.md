# AGENTS.md — chart_runtime

## Layer mission
`chart_runtime` is the primary product of this repository.
It owns:
- chart readers/normalizers
- SENC v1 build/read/validate
- unified internal models
- viewport state
- scene construction
- render core
- chart catalog / coverage / quilt / zoom policy
- portrayal / symbolization
- picking/query core

It must remain reusable and hostable.

## Public API rules
- Prefer C API + opaque handles.
- Do not expose QWidget, QWindow, QMainWindow, QRhi in runtime public ABI.
- Do not expose parser internals just for convenience.

## Separation rules
- Parsing and rendering must stay separate.
- SENC remains central in Phase 1.
- Render core must not become UI-aware.
- Scene and viewport must remain explicit.

## Phase 1 focus
Must focus on:
- S57 / CM93 / S-101 single-chart
- full SENC v1 for those cases
- runtime-side rendering pipeline

Must not expand into:
- quilting
- zoom policy
- full nautical symbology
- S-102
- MBTiles
