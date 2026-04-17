# Current Iteration

- Task: `96-s52-lookup-and-csp-family-sweep`
- Status: `95-s52-instruction-parser-and-compiler-coverage` expanded the `chartsymbols.xml` parser/compiler path so `AC(...)` now enters typed IR, synthetic `LS_*` line instructions synthesize compiled line-style assets instead of silently degrading, and the committed Phase 6B inventory now reports zero unsupported instruction tokens while keeping remaining renderer/CSP gaps explicit.
- Blocker: `none`
- Previous task: `95-s52-instruction-parser-and-compiler-coverage` updated the parser/compiler baseline, refreshed `tests/data/reference/phase6b_s52_resource_snapshot_inventory.reference.json`, and verified the focused parser/compiler suite without widening the runtime ABI or touching host code.
