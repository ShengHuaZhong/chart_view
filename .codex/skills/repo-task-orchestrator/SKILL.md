# repo-task-orchestrator

## Purpose

Drive this repository strictly as a task-based engineering program.

This skill is the default top-level skill for any coding task in this repo.
Use it to determine the real task start point, enforce task boundaries, keep state files truthful, and stop cross-task contamination.

## Use this skill when

- A user asks to continue work on the repository
- A user asks to execute one or more task files
- There is any ambiguity between `AGENTS.md`, `plan.md`, `state/*`, recent commits, and task files
- You need to decide whether the current work is a new task, a blocker audit, a verification pass, or a closeout

## Read first, in this order

1. `AGENTS.md`
2. `plan.md`
3. `docs/architecture.md`
4. `docs/coding_rules.md`
5. `docs/build_environment.md`
6. `state/current_iteration.md`
7. `state/done.md`
8. `state/blocked.md`
9. The current task file
10. Any directly dependent task files, only if needed for context

## Core rules

- One task at a time.
- Do not silently combine multiple task files into one change.
- Before editing, explicitly state what the current task will **not** modify.
- Keep the repository DLL-first:
  - `chart_runtime.dll`
  - `chart_qtwidgets.dll`
  - `chart_standalone.exe`
- Keep runtime ABI narrow:
  - C API
  - opaque handles
  - DTO-style structs
- Do not expose `QWidget`, `QMainWindow`, `QRhi`, `PROJ` handles, parser internals, deep render classes, or font internals in public runtime ABI.
- Do not move parsing, SENC, scene internals, portrayal, chart selection, quilt policy, or GPU ownership into host layers.

## How to decide the real current task

When files disagree, use this precedence:

1. Current worktree
2. `state/current_iteration.md`
3. `state/done.md`
4. Recent local commits
5. Existing task files
6. `plan.md`
7. `AGENTS.md`

Do not get stuck because older files lag behind.
Make the most conservative grounded decision and say why.

## Required per-task workflow

For every formal task:

1. Identify the selected task.
2. State why this task is next.
3. State boundaries: what will not be modified.
4. Implement only the required slice.
5. Run the minimum required verification.
6. Update:
   - `state/current_iteration.md`
   - `state/done.md`
   - `state/blocked.md` if blocked
7. Create exactly one git commit for the completed task, only after verification passes.

## If the task is blocked

- Do not mark it complete.
- Do not update `done.md` as if complete.
- Do not create a completion commit.
- Update `state/blocked.md` with:
  - technical reason
  - attempts made
  - verification evidence
  - why the task cannot be completed without widening scope

## Required output format

At task start:

1. `Selected task`
2. `Why this task now`
3. `Scope understood`
4. `What this task will not modify`
5. `Most likely root cause` or `N/A`
6. `Implementation plan`

At task end:

1. `Selected task`
2. `Why this task now`
3. `Scope understood`
4. `What this task did not modify`
5. `Implementation changes made`
6. `Verification run`
7. `State files updated`
8. `Git commit created`
9. `Recommended next task`

## Anti-patterns

Do not:
- start the next task “while already here”
- hide blocker work inside a completion task
- commit before tests pass
- create more than one completion commit per finished task
- treat OpenCPN as the normative specification