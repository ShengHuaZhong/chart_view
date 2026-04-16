# Architecture

## Product structure
- `chart_runtime.dll` — chart engine core
- `chart_qtwidgets.dll` — Qt Widgets bridge / host integration
- `chart_standalone.exe` — official host shell

## Layer map
- `domain_core`: routes, waypoints, vessel state, selection
- `chart_data`: S57 / CM93 / S-101 readers and normalizers
- `senc`: SENC v1 build/read/validate
- `render_core`: Qt 6 RHI backend, render scheduler, layer renderers
- `portrayal`: symbol rules and full nautical symbology
- `qtwidgets`: `ChartViewWidget`, input bridge, viewport binding
- `standalone`: menus, dock panels, status bar, host wiring

## Phase 1 flow
source chart
-> reader
-> normalizer
-> unified dataset
-> SENC v1
-> scene build
-> runtime render
-> Qt host display

## Phase 2 flow
SENC set
-> ChartCatalog
-> CoverageIndex
-> QuiltPlanner
-> ZoomPolicy
-> MultiChart SceneSnapshot
-> render

## Phase 3 flow
Feature dataset
-> Symbolizer / PortrayalRegistry
-> display priority / layer grouping
-> point/line/area/text rendering

## Phase 4 portrayal boundary
- `chart_runtime` owns the S-52 portrayal baseline, including presentation-asset adapters, lookup logic, conditional symbology decisions, and render instructions.
- IHO S-52 / Annex A / S-64 are the normative source for portrayal behavior and asset intent.
- OpenCPN may be used only as an engineering reference or behavior cross-check; it is not the normative source and its code or packaging must not be copied into this repository.
- `chart_qtwidgets` and `chart_standalone` host the resulting runtime output but must not own S-52 asset tables, lookup decisions, or conditional symbology policy.
