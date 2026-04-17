# Current Iteration

- Task: `86a-full-lookup-and-instruction-string-coverage`
- Status: `85a-opencpn-resource-compiler-to-ir` completed by compiling the vendored OpenCPN resource snapshot into the preferred deterministic `S52CompiledCatalog`, preserving richer lookup/source metadata in compiled rows, and switching the runtime's asset/lookup/rule-enumeration path to prefer the compiled OpenCPN-resource catalog while keeping the built-in private catalog as typed-IR and asset fallback only.
- Blocker: `none`
- Previous task: `85a-opencpn-resource-compiler-to-ir` established the preferred OpenCPN-resource compiled catalog path only; broad instruction-string parsing and real-chart coverage expansion remain explicitly in `86a-full-lookup-and-instruction-string-coverage`.
