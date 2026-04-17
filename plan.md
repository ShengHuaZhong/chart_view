# plan.md

## Project mission

Build a DLL-first marine chart system whose primary product is `chart_runtime.dll`, with:

- `chart_runtime.dll` as the reusable chart-engine core
- `chart_qtwidgets.dll` as the Qt Widgets integration layer
- `chart_standalone.exe` as the official demo host

The repository is engine-first, not app-first.

## Architecture rules

- Preserve the product hierarchy:
  - `chart_runtime.dll`
  - `chart_qtwidgets.dll`
  - `chart_standalone.exe`
- Keep Qt Widgets as the shell and Qt 6 RHI as the main rendering backend.
- Keep parsing, SENC, scene construction, portrayal, chart selection, quilting, and GPU ownership inside `chart_runtime`.
- Keep the runtime public ABI narrow:
  - C API
  - opaque handles
  - DTO-style structs
- Do not expose `QWidget`, `QMainWindow`, `QRhi`, PROJ handles, parser internals, deep render classes, or font-engine internals in the runtime ABI.

## Phase roadmap

### Phase 1
- Single-chart rendering through the full SENC v1 pipeline
- S57 / CM93 / S-101 single-chart validation

### Phase 2
- Chart catalog, coverage, quilt planning, and zoom policy
- Multi-chart validation

### Phase 3
- Generic semantic portrayal baseline
- Point / line / area / text semantic symbolization baseline

### Phase 4
- PROJ-backed projected quilting
- S-52-based S57 display baseline
- Unicode / multilingual text baseline

### Phase 5
- S57-first source/update ingest
- SENC v2 semantic persistence
- Compiled private S-52 catalog/rule execution baseline
- Runtime mariner/filter/query APIs
- Broader real-chart S57 validation and host control bindings

### Phase 6
- Official Annex A-driven full S-52 graphics engine for S57
- Offline compilation of vendored official Annex A digital assets
- Chart 1 / S-64 graphical reference harnesses
- OpenCPN visual-delta harness as engineering reference only

## Current repository status

- Tasks through `81-phase5-demo-verification` are complete.
- The current verified baseline is the documented Phase 5 S57-first runtime baseline.
- The next phase begins with:
  - `82-repo-truth-sync-for-phase6`
  - `83-s52-annexa-asset-ingest-core`
  - `84-s52-offline-catalog-compiler`
  - `85-s52-full-lookup-coverage-s57`
  - `86-s52-csp-engine-and-rule-vm`
  - `87-s52-point-symbol-engine`
  - `88-s52-line-style-and-area-pattern-engine`
  - `89-s52-text-annotation-engine`
  - `90-s52-display-modes-and-mariner-settings-complete`
  - `91-chart1-and-s64-reference-harness`
  - `92-opencpn-visual-delta-harness`
  - `93-phase6-full-s52-engine-verification`

## Phase 6 guardrails

- Normative truth for the S57 graphics engine is:
  - IHO S-52 6.1.1
  - Annex A 4.0.4
  - S-64 3.0.3
- Vendor the official Annex A digital asset sources into this private repository.
- Use a hybrid regression policy:
  - commit focused goldens, crops, manifests, and object-level assertions
  - keep heavyweight captures and reproducible large artifacts optional
- Keep Phase 6 S57-first.
- Do not turn engineering completion into S-64 or ECDIS compliance claims.

## Task execution rules

- One task at a time.
- State what the task will not modify before editing.
- Update:
  - `state/current_iteration.md`
  - `state/done.md`
  - `state/blocked.md` when blocked
- Run the minimum required verification for every task.
- Create exactly one git commit per completed task, and only after verification passes.
