# 00 — Repository bootstrap

## Objective
runtime / qtwidgets / standalone build skeleton

## Phase
- Phase 1

## Layer
- build

## Depends on
- none

## In scope
- Create layered repository skeleton
- Top-level CMake with runtime/qtwidgets/standalone/test options
- Basic directory scaffolding

## Out of scope
- Do not implement chart features

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Create layered repository skeleton
- Top-level CMake with runtime/qtwidgets/standalone/test options
- Basic directory scaffolding

## Deliverables
- Root CMakeLists.txt
- subdir CMakeLists.txt placeholders
- basic README updates

## Done when
- Three targets configure successfully
- Directory structure matches DLL-first layout

## Verification
- Configure succeeds for all enabled targets

## Notes
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
