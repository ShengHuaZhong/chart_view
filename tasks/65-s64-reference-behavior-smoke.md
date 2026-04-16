# 65 — S-64 reference behavior smoke

## Objective
Reference-behavior smoke aligned with selected S-64 expectations

## Phase
- Phase 4

## Layer
- verification / portrayal behavior

## Depends on
- 64

## In scope
- Add focused behavior checks inspired by selected S-64 verification expectations
- Cover a narrow but explicit subset of display-mode / conditional / portrayal behaviors
- Keep the scope suitable for repository smoke validation rather than full type approval

## Out of scope
- No claim of full S-64 pass / compliance
- No unrelated chart-format expansion
- No host-owned validation logic

## Inputs
- AGENTS.md
- docs/architecture.md
- docs/coding_rules.md
- docs/build_environment.md
- docs/phase_roadmap.md

## Required changes
- Define the selected S-64-inspired behavior subset in repository docs or test comments
- Add targeted smoke or golden tests for that subset
- Keep the normative-source boundary clear

## Deliverables
- S-64-inspired behavior smoke coverage
- Baseline documentation for the selected subset

## Done when
The repository can automatically verify a narrow, explicit subset of S-64-inspired portrayal behavior for the Phase 4 S57 baseline.

## Verification
- Reference-behavior smoke tests

## Notes
- Be explicit about what is covered and what is not.
- Do not overstate compliance.
- Preserve DLL-first boundaries.
- State what this task will not modify before editing.
- Update `state/current_iteration.md` and `state/done.md` after completion.
