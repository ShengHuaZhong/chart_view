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
