# 63 — Multilingual label selection and projected layout

## Objective
Multilingual label selection and projected label placement

## Phase
- Phase 4

## Layer
- text / layout / scene

## Depends on
- 62
- 54

## In scope
- Add runtime-owned label-selection policy for multilingual names
- Lay out labels in projected display space so label positions stay coherent with projected charts and quilt patches
- Add a baseline collision / overlap policy suitable for the repository baseline

## Out of scope
- No full production text-placement engine
- No full bidi / advanced shaping compliance promise yet
- No host-owned label-layout logic

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add language / name selection helpers
- Upgrade label placement to projected display-space anchoring
- Add multilingual selection and projected layout tests

## Deliverables
- Multilingual label-selection policy
- Projected label-layout baseline
- Selection / layout tests

## Done when
The runtime can choose among multilingual label candidates and place the chosen label in projected display space instead of relying on pre-Phase-4 local heuristics.

## Verification
- Projected label-layout tests
- Multilingual label-selection tests
- Updated label or feature-renderer coverage

## Notes
- Keep naming policy explicit and deterministic.
- Stay honest about baseline collision handling scope.
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
