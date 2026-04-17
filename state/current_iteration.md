# Current Iteration

- Task: `85a-opencpn-resource-compiler-to-ir`
- Status: `84a-chartsymbols-parser-and-source-model` completed by adding a runtime-internal `OpenCpnChartsymbolsParser`, expanding `S52SourceCatalog` to retain richer OpenCPN source metadata, and verifying the vendored `chartsymbols.xml` snapshot parses into the expected section counts and representative entries without switching the runtime to the compiled OpenCPN path yet.
- Blocker: `none`
- Previous task: `84a-chartsymbols-parser-and-source-model` added the Phase 6A parser/source-model layer only; deterministic compilation and runtime preference for the parsed OpenCPN-resource catalog remain explicitly in `85a-opencpn-resource-compiler-to-ir`.
