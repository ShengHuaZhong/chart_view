# 55 — Projected quilt seams and patch clipping

## Objective
Projected quilt patches with seam-safe clipping

## Phase
- Phase 4

## Layer
- quilt / clipping

## Depends on
- 54

## In scope
- Make quilt patch clipping operate in projected display space
- Add seam-aware patch boundary handling for adjacent charts
- Keep reference-chart ordering and patch ownership explicit
- Add focused tests around chart-edge overlap and clipping

## Out of scope
- No S-52 portrayal assets yet
- No Unicode label placement yet
- No full production anti-seam raster polish

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Update quilt patch generation to use projected polygons or projected clip regions
- Add seam / overlap regression coverage
- Preserve existing chart-selection policy boundaries

## Deliverables
- Projected quilt patch clipping path
- Seam regression tests

## Done when
The runtime can build projected quilt patches without relying on purely geographic clipping heuristics.

## Verification
- Quilt planner tests
- Projected seam / clipping tests
- Targeted multi-chart smoke if available

## Notes
- PROJ handles transforms only; seam behavior remains a runtime responsibility.
- Keep this task focused on projected patch generation.
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
