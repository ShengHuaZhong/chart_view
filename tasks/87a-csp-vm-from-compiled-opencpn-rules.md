# 87a - CSP VM from compiled OpenCPN rules

## Objective
Upgrade the runtime-owned CSP engine so it executes compiled conditional opcodes derived from OpenCPN resources while remaining under `chart_runtime` control.

## Phase
- Phase 6A

## Layer
- runtime portrayal conditional engine

## Depends on
- 86a

## In scope
- Compile conditional tokens into VM opcodes
- Mariner-settings-sensitive CSP execution
- Reuse existing query/explain surface where needed

## Out of scope
- No host logic changes
- No public ABI redesign
