# Current Iteration

- Task: `76-s57-query-inspection-and-rule-explain`
- Status: `75-full-s52-renderer-integration-s57 completed; chart_runtime now executes the compiled S-52 point and area instruction variants from the normal S57 render path, including SCAMIN-aware suppression and conditional sector-light/depth style routing through the runtime-owned renderer instead of depending on the earlier generic fallback as the primary result`
- Blocker: `none active`
- Previous task: `75-full-s52-renderer-integration-s57` completed by teaching `FeatureLayerRenderer` to consume conditional S-52 instruction outputs when resolving rendered point and area styles, registering the corresponding runtime-owned portrayal variants, and adding focused renderer plus SENC-backed symbolized smoke coverage for SCAMIN suppression and conditional sector-light/depth rendering without widening into query/filter or host logic.
