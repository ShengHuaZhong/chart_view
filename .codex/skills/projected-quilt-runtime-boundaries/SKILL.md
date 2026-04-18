# projected-quilt-runtime-boundaries

## Purpose

Protect the repository’s runtime-only ownership of projection, coverage, quilting, patch clipping, and seam handling.

Use this skill whenever projected quilting or quilt-plan behavior is being touched.

## Use this skill when

- Editing projected coverage logic
- Editing quilt planning
- Editing patch clipping / seam handling
- Debugging chart selection interactions with projected display space
- Auditing whether host layers are taking on runtime responsibilities

## Read first

1. `AGENTS.md`
2. `plan.md`
3. `docs/architecture.md`
4. The current task file
5. Relevant runtime projection/quilt/coverage files
6. Related tests

## Hard architectural rules

- PROJ is private to `chart_runtime`.
- Projection solves coordinate transforms only.
- Coverage resolution, chart selection, patch clipping, seam handling, and quilt policy remain runtime responsibilities.
- One frame / one quilt plan must use one common display projection.
- Do not let each chart effectively render in its own unrelated projection space.
- Host layers must not absorb quilt logic.

## Design checklist

For any change, explicitly answer:

1. What is the common display projection?
2. Where are chart inputs transformed into that space?
3. Where is coverage computed?
4. Where is quilt ordering decided?
5. Where is patch clipping performed?
6. Where is seam handling performed?
7. Does any of this leak into host code or ABI?

## Verification expectations

Prefer tests that prove:
- both charts survive into the intended quilt plan when they should
- projected overlap is computed in the intended space
- patch clipping does not accidentally erase required detail coverage
- resize and host presentation do not change runtime ownership boundaries

## Anti-patterns

Do not:
- solve quilt bugs by moving logic into the widget or host
- leak `PROJ` handles through the public API
- fake quilt success by disabling patch subtraction globally
- hardcode specific chart ids as permanent quilt exceptions