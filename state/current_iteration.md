# Current Iteration

- Task: `74-complete-conditional-symbology-engine`
- Status: `73-full-mariner-settings-runtime-api completed; chart_runtime now exposes narrow Phase 5 S-52 mariner-settings, object-class filter, rule-filter, and compiled-rule enumeration surfaces through the public C API without leaking renderer or host internals`
- Blocker: `none active`
- Previous task: `73-full-mariner-settings-runtime-api` completed by extending the public runtime DTO/C API with mariner-settings and filter controls, storing that state inside `RuntimeContext`, and wiring the runtime-owned renderer/symbolizer settings channel to consume the configured S-52 display settings without expanding into full conditional-engine behavior or host UI binding.
