# Six-phase Roadmap

## Phase 1 - Single-chart rendering through SENC v1
Deliver:
- `chart_runtime.dll`
- `chart_qtwidgets.dll`
- `chart_standalone.exe`
- S57 / CM93 / S-101 single-chart rendering
- full SENC v1 pipeline

## Phase 2 - Quilting and zoom
Deliver:
- ChartCatalog
- CoverageIndex
- QuiltPlanner
- ZoomPolicy
- Multi-chart scene build and render
- S57 / CM93 / S-101 multi-chart smoke validation

## Phase 3 - Generic semantic portrayal baseline
Deliver:
- PortrayalRegistry
- FeatureSymbolizer
- display priority and layering
- point / line / area / text semantic symbolization baseline
- S57 and S-101 symbolized smoke validation

## Phase 4 - Projected quilting, S-52-based S57 display, and Unicode / multilingual text
Deliver:
- PROJ-backed display projection inside `chart_runtime`
- projected coverage, projected scene construction, and projected quilt patch clipping
- S-52 presentation assets, lookup, display settings, and conditional symbolization for S57
- runtime-owned Unicode text system, font fallback, glyph cache, and multilingual label selection
- integrated real-chart S57 smoke validation for projected quilting + S-52 + Unicode labels
- Phase 4 verification notes describing the achieved baseline and remaining non-compliance scope

## Phase 5 - S57-first full display chain and data capability
Deliver:
- runtime-owned `S57SourceModel`, source manifests, and update manifests
- update application for base `.000` plus `.001+` ENC sequences
- SENC v2 for richer S57 semantics and applied-update persistence while keeping v1 compatibility
- private compiled S-52 catalog, fuller lookup / rule IR, and expanded conditional symbology
- narrow runtime C API for mariner settings, object/rule filtering, and feature inspection
- fuller S57-first renderer execution with object-class and rule-level selection controls
- OpenCPN engineering parity harness and broader real-chart smoke coverage
- Phase 5 verification notes describing the achieved baseline and remaining non-compliance scope

## Phase 6 - Official Annex A-driven full S-52 graphics engine
Deliver:
- repository truth sync for the post-Phase-5 state and the new Phase 6 task chain
- vendored official Annex A / Chart 1 / S-64 source assets with provenance manifests
- offline compiler pipeline that turns official digital assets into deterministic runtime catalogs and IR
- fuller S57 semantic retention and lookup coverage aligned to the official asset set
- CSP / rule-VM execution driven by official compiled metadata
- dedicated point, line/area, and text/annotation S-52 graphics engines
- complete mariner display modes and settings on the narrow runtime C API
- Chart 1 / S-64 graphical reference harnesses with screenshot or tile diff evidence
- OpenCPN visual-delta harness as an engineering comparison only
- final Phase 6 verification notes describing the achieved graphics-engine baseline and the remaining non-compliance scope

## Current repository status
- Phases 1 through 5 are complete at their documented repository baselines.
- Phase 6 is the next planned phase.
