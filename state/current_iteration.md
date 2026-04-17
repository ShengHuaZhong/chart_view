# Current Iteration

- Task: `87a-csp-vm-from-compiled-opencpn-rules`
- Status: `86a-full-lookup-and-instruction-string-coverage` completed by parsing `chartsymbols.xml` instruction strings into typed IR, widening compiled-rule lookup selection beyond the old selected subset, and validating on real S57 charts that the preferred OpenCPN-derived compiled catalog now produces non-zero lookup hits, text-instruction hits, and deterministic fallback rows only when the richer compiled path cannot supply renderable instructions.
- Blocker: `none`
- Previous task: `86a-full-lookup-and-instruction-string-coverage` established broader compiled lookup coverage and offline instruction-string parsing only; CSP opcode compilation and mariner-settings-sensitive conditional execution remain explicitly in `87a-csp-vm-from-compiled-opencpn-rules`.
