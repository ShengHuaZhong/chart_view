# Current Iteration

- Task: `77-s57-class-and-rule-selection-controls`
- Status: `76-s57-query-inspection-and-rule-explain completed; chart_runtime now exposes DTO-based feature query and describe surfaces that return runtime-owned feature summaries, selected names, and active compiled S-52 rule explanations without exposing parser or renderer internals`
- Blocker: `none active`
- Previous task: `76-s57-query-inspection-and-rule-explain` completed by adding narrow feature-query and feature-describe C APIs, wiring them through `RuntimeContext` against the loaded S57/quilt datasets, and backing them with focused runtime API verification for point queries, selected names, and active-rule explanations without widening into host UI or task-77 filter behavior.
