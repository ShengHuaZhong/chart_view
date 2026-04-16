# Current Iteration

- Task: `78-opencpn-parity-harness-and-reference-samples`
- Status: `77-s57-class-and-rule-selection-controls completed; chart_runtime now honors object-class and stable compiled-rule selection controls inside the runtime symbolization/query/render path, so S57 symbol content can be selectively suppressed without host-owned rule logic`
- Blocker: `none active`
- Previous task: `77-s57-class-and-rule-selection-controls` completed by wiring the existing runtime filter DTO state into `FeatureSymbolizer`, propagating that behavior through `FeatureLayerRenderer` and the task-76 query surface, and adding focused precedence coverage for object-class suppression, stable rule-id suppression, and the resulting runtime render/query behavior without widening into host UI or later parity/smoke work.
