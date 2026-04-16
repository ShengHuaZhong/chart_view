# 62 — Font fallback and glyph cache

## Objective
Runtime-owned font fallback and glyph cache for Unicode labels

## Phase
- Phase 4

## Layer
- text / font / rendering

## Depends on
- 61

## In scope
- Add runtime-owned font fallback policy
- Add glyph cache / glyph atlas or equivalent caching path for Unicode label rendering
- Handle missing-glyph fallback without host-owned text rendering

## Out of scope
- No language-preference label selection yet
- No full advanced shaping compliance promise yet
- No host-side font engine ownership

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add fallback font selection and glyph-cache sources
- Add focused tests for glyph availability / fallback behavior
- Upgrade label rendering tests to cover non-ASCII text

## Deliverables
- Font fallback path
- Glyph cache path
- Multiscript glyph tests

## Done when
The runtime can render non-ASCII labels through font fallback instead of dropping them or producing tofu for the baseline test set.

## Verification
- Glyph-cache tests
- Font-fallback tests
- Updated label-render tests

## Notes
- Choose a baseline that is realistic for repository-controlled tests.
- Keep host code out of glyph ownership.
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
