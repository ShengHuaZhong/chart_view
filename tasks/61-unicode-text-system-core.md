# 61 — Unicode text system core

## Objective
Runtime-owned Unicode text system baseline

## Phase
- Phase 4

## Layer
- text / runtime

## Depends on
- 60

## In scope
- Replace ASCII-oriented or tiny-glyph assumptions in the current label path with a Unicode-capable internal text model
- Add UTF-8 / Unicode-safe label extraction and storage through runtime-owned DTOs or internals
- Keep text ownership inside `chart_runtime`

## Out of scope
- No font fallback yet
- No multilingual label-selection policy yet
- No full complex-script compliance promise yet

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Add Unicode-safe text types or helpers
- Upgrade label extraction and intermediate label storage
- Add focused Unicode text-path tests

## Deliverables
- Unicode text core
- Unicode label-path tests

## Done when
The runtime text path no longer assumes ASCII-only or single-byte-safe label content.

## Verification
- Unicode extraction / storage tests
- Updated label tests

## Notes
- Do not treat `wchar_t` plumbing by itself as task completion.
- Keep the font engine details private.
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
