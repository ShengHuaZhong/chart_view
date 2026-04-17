# 85a - OpenCPN resource compiler to IR

## Objective
Compile the richer OpenCPN-derived source catalog into deterministic `chart_view` compiled catalog / IR output and make that path the preferred runtime source.

## Phase
- Phase 6A

## Layer
- runtime portrayal compiler

## Depends on
- 84a

## In scope
- Deterministic compilation from parsed OpenCPN resources
- Stable `rule_id` generation
- Runtime preference for compiled OpenCPN-resource output

## Out of scope
- No full instruction-string parsing yet
- No CSP engine upgrade yet
