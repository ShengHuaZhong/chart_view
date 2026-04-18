# windows-cpp-qt-build-and-verify

## Purpose

Build, test, and verify this repository on Windows/MSVC/Qt/CMake with reproducible commands.

This skill is for any task that touches C++, CMake, Qt runtime behavior, test targets, or standalone host smoke verification.

## Use this skill when

- Editing C++ source or headers
- Updating CMake targets or dependencies
- Running `ctest`
- Running direct standalone host smoke
- Verifying runtime / qtwidgets / standalone integration

## Read first

1. `docs/build_environment.md`
2. `CMakePresets.json`
3. Top-level `CMakeLists.txt`
4. The current task file
5. Related test `CMakeLists.txt`
6. `state/current_iteration.md`

## Default build assumptions

- Windows
- MSVC toolchain
- CMake preset-driven workflow
- Qt 6 available through configured toolchain/prefix path
- Tests are part of the required evidence, not optional

## Standard workflow

1. Identify the narrowest build targets needed for the current task.
2. Build only those targets first.
3. Run focused `ctest` filters first.
4. If required by the task, run a direct standalone real-chart host smoke.
5. Capture exact commands and exact outcomes.

## Command style

Prefer explicit commands such as:

- `cmake --build --preset <preset> --target <targets...>`
- `ctest --test-dir <build-dir> -C <config> --force-new-ctest-process -R "<regex>" --output-on-failure`

If the task requires standalone host proof, also run the actual executable directly and report:
- whether the window is visibly non-blank
- whether resize preserves center
- what presentation path is currently used

## Verification reporting requirements

Always report:

- exact build command
- exact test command
- key pass/fail result
- whether all required checks passed
- any warnings that are non-blocking but still relevant

## Runtime/host verification checklist

When host verification is required, report:

- chart path used
- chart type used
- whether open succeeded
- whether render result is non-blank
- whether resize preserves viewport center
- what path is presenting pixels to screen
- whether there are environment warnings such as missing `proj.db`

## Anti-patterns

Do not:
- claim success without running the commands
- run the full world if the task only needs focused targets
- hide test failures behind vague summaries
- treat build success as task success when runtime smoke is required