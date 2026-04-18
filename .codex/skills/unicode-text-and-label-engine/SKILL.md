# unicode-text-and-label-engine

## Purpose

Handle Unicode-safe text flow, name selection, font fallback, glyph caching, and label rendering quality as a real engine feature, not as a narrow encoding patch.

## Use this skill when

- Editing label extraction or label layout
- Editing font fallback or glyph cache
- Editing text rendering
- Debugging unreadable labels, placeholder glyphs, or “garbled” CJK output
- Adding multilingual label behavior

## Read first

1. `AGENTS.md`
2. `plan.md`
3. Current task file
4. Runtime text / label / glyph files
5. Any current Phase 4+ text verification docs
6. Relevant real-chart smoke tests

## Core definition of “done”

Unicode / multilingual support is not complete when:
- only ASCII works
- only one font works
- strings exist internally but render as squares or blobs
- labels render through placeholder glyphs without visibility into that fact

It is only complete when:
- text is stored and passed safely as Unicode
- label selection policy is explicit
- font fallback works
- glyph caching works
- rendered labels are visibly readable enough for the task’s target baseline

## Required subproblems

Always think in these layers:

1. source attribute extraction
2. label selection policy (`NOBJNM`, `OBJNAM`, etc.)
3. Unicode-safe storage and transport
4. font fallback
5. glyph cache / atlas
6. text rendering quality
7. label placement and priority

## Quality rules

Prefer:
- alpha-aware glyph blitting
- measurable placeholder-glyph reporting
- fallback visibility
- explicit label-source reporting

If the current task is only about baseline support, say so.
Do not overclaim full text engine quality if rendering is still crude.

## Verification expectations

Report at least:
- total label candidates
- Unicode-capable label candidates
- visible projected labels
- whether placeholder glyphs were used
- whether the chosen name source was `OBJNAM`, `NOBJNM`, or other

## Anti-patterns

Do not:
- reduce the task to `wchar_t` plumbing
- hide placeholder-glyph usage
- claim multilingual support because the parser kept the bytes
- mix font-engine internals into runtime public ABI