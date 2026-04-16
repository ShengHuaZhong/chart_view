# Current Iteration

- Task: `73-full-mariner-settings-runtime-api`
- Status: `72-complete-s52-lookup-and-rule-ir completed; chart_runtime now resolves S-52 lookup rows through a compiled-catalog-backed typed rule/instruction IR with stable rule ids, display metadata, and runtime-owned instruction variants`
- Blocker: `none active`
- Previous task: `72-complete-s52-lookup-and-rule-ir` completed by replacing the narrow hardcoded lookup subset with compiled-catalog-backed `S52LookupModel` results, adding typed instruction IR for point/line/area/text/conditional rules, and wiring the existing symbolizer/conditional paths to consume those typed instructions without widening into mariner settings API or renderer integration work.
