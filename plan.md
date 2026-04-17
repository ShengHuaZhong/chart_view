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
- Approved fallback execution path: `chartsymbols.xml`-first / OpenCPN `s57data` resource snapshots compiled into `chart_view` IR when the official raw-asset path is blocked
- Current completion policy for the fallback path:
  - finish the remaining lookup rows with GPL-compatible OpenCPN supplemental resource files first
  - use repo-owned manual overlay assets only for residual gaps that remain absent after the upstream supplement pass

## Current repository status

- Tasks through `81-phase5-demo-verification` are complete.
- The current verified baseline is the documented Phase 5 S57-first runtime baseline.
- `82-repo-truth-sync-for-phase6` is complete.
- `83-s52-annexa-asset-ingest-core` is a preserved historical blocker for the official raw Annex A asset path.
- The Phase 6A fallback chain completed through:
  - `83a-opencpn-resource-bundle-ingest`
  - `84a-chartsymbols-parser-and-source-model`
  - `85a-opencpn-resource-compiler-to-ir`
  - `86a-full-lookup-and-instruction-string-coverage`
  - `87a-csp-vm-from-compiled-opencpn-rules`
  - `88a-point-line-area-engine-from-compiled-assets`
  - `89a-text-annotation-engine-from-compiled-rules`
  - `90a-display-modes-and-mariner-settings-complete`
  - `91a-chart1-s64-graphical-reference-harness`
  - `92a-opencpn-visual-delta-harness`
  - `93a-phase6a-verification`
- The post-`93a` display-completeness follow-up tightened the fixed Phase 6A scenes for `BOYSPP`, `SOUNDG`, `WRECKS`, `LNDARE`, and `FAIRWY` while keeping the runtime ABI unchanged.
- The Phase 6B symbol-coverage chain `94-99` is complete.
- The Phase 6C wave-1 compiler-first chain `100-104` is complete.
- The next active chain is Phase 6D:
  - `105-phase6d-truth-sync-and-task-chain`
  - `106-s52-upstream-supplemental-opencpn-assets`
  - `107-s52-manual-overlay-asset-pack`
  - `108-s52-all-lookuprow-closure`
  - `109-s52-harness-and-standalone-proof`
  - `110-phase6d-all-lookuprow-verification`

## Phase 6D lookup-row completion policy

- Goal:
  - drive the pinned `Release_5.14.0` fallback path to full `lookupRowsTotal` coverage without widening the runtime ABI or changing host ownership boundaries
- Resource resolution order:
  1. existing vendored `Release_5.14.0/s57data` assets
  2. GPL-compatible vendored supplemental OpenCPN resource files and atlases
  3. repo-owned manual overlay assets for residual IDs that remain absent after step 2
- OpenCPN supplemental candidates to search and vendor first:
  - `BOYLAT52`
  - `BOYLAT53`
  - `BOYLAT54`
  - `BOYLAT55`
  - `BOYLAT56`
  - `BOYSPP50`
  - `VEHTRF01`
  - `BCNCON81`
  - `ARCSLN01`
  - `DANGER53`
  - `BOYSPR02`
  - `BOYSPR03`
  - `NEWOBJ01` only if a real upstream asset exists instead of parser/compiler noise
- Manual overlay candidates to reserve for the residual pass:
  - `FLTHAZ02`
  - `BOYSPH79`
  - `ESSARE01`
  - `PSSARE01`
  - any other residual asset IDs proven absent after the supplemental OpenCPN sweep
- Parser/compiler cleanup items that must not be "fixed" by drawing fake assets first:
  - `TOPSHP73TESOBJNAM...`
  - `DGPS01DRFSTA01`
  - `TOWERS74TXOBJNAM...`
  - similarly malformed spillover IDs

## Phase 6 guardrails

- Normative truth for the S57 graphics engine is:
  - IHO S-52 6.1.1
  - Annex A 4.0.4
  - S-64 3.0.3
- Prefer vendored official Annex A digital asset sources when available.
- When the official raw-asset path is blocked, vendor a fixed OpenCPN `data/s57data` snapshot as an engineering input source and compile it into `chart_view`-owned IR.
- Use a hybrid regression policy:
  - commit focused goldens, crops, manifests, and object-level assertions
  - keep heavyweight captures and reproducible large artifacts optional
- Keep Phase 6 S57-first.
- Do not turn engineering completion into S-64 or ECDIS compliance claims.
- Do not directly link or embed OpenCPN `s52plib`.
- For Phase 6D task files, require the explicit first-read order and the post-change verification/report/update checklist used by the new `105-110` task chain.

## Task execution rules

- One task at a time.
- State what the task will not modify before editing.
- Update:
  - `state/current_iteration.md`
  - `state/done.md`
  - `state/blocked.md` when blocked
- Run the minimum required verification for every task.
- Create exactly one git commit per completed task, and only after verification passes.
