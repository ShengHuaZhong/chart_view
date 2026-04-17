# Current Iteration

- Task: `97-s52-renderer-asset-family-completion`
- Status: `96-s52-lookup-and-csp-family-sweep` expanded the Phase 6B family sweep without widening the runtime ABI: line-family lookup ranking now explicitly prefers `Lines` rows, newly covered family CSP tokens map to stable opcodes, and suppressed display-category / SCAMIN paths preserve compiled instructions and `conditionIds` for explain-surface parity.
- Blocker: `none`
- Previous task: `96-s52-lookup-and-csp-family-sweep` tightened family-specific lookup ranking and conditional-opcode coverage for the chosen line/restricted/topmark families, while the broader `runtime.s57_lookup_coverage_smoke` crash remains an out-of-scope real-chart regression to revisit with task 98's expanded real-chart evidence.
