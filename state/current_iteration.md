# Current Iteration

- Task: `88a-point-line-area-engine-from-compiled-assets`
- Status: `87a-csp-vm-from-compiled-opencpn-rules` completed by compiling OpenCPN-derived conditional tokens into runtime-owned opcodes, normalizing conditional instructions on lookup results, executing mariner-settings-sensitive depth/light conditional outputs through the runtime CSP layer, and validating that renderer-facing symbolization preserves compiled conditional opcodes while staying inside `chart_runtime`.
- Blocker: `none`
- Previous task: `87a-csp-vm-from-compiled-opencpn-rules` closed the opcode-compilation and conditional-execution layer only; point/line/area asset metadata still needs to drive richer runtime drawing behavior in `88a-point-line-area-engine-from-compiled-assets`.
