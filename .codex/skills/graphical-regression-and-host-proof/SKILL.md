# graphical-regression-and-host-proof

## Purpose

Provide graphical evidence, scene proof, and direct standalone host proof for portrayal and rendering changes.

This skill exists because counter-based smoke is not enough for Phase 6+ acceptance.

## Use this skill when

- Working on tasks with graphical regression requirements
- Updating fixed scenes, crops, or family metrics
- Extending host proof
- Comparing against Chart 1 / S-64 reference behavior
- Building evidence that a row or family is graphically covered

## Read first

1. `AGENTS.md`
2. `plan.md`
3. Current task file
4. Existing graphical verification docs
5. Current fixed-scene/reference manifests
6. Standalone host smoke entry points
7. Any existing parity/delta harness docs

## Core principle

Counter-based smoke may remain, but it is not enough as primary acceptance evidence for advanced portrayal work.

Prefer:
- focused goldens
- fixed-scene crops
- family metrics
- direct standalone host evidence
- explicit rule/scene/object assertions

## Required proof types

Choose the smallest set that honestly proves the task, for example:

- fixed-scene render output
- cropped symbol/line/pattern evidence
- family-level counts
- direct real-S57 standalone host run
- viewport-specific evidence
- label visibility evidence

## Direct host proof requirements

When a task requires host proof, report:

- exact executable run
- real chart path
- window non-blank or blank
- viewport center behavior under resize
- current presentation path
- any relevant runtime warnings

## Chart 1 / S-64 usage

For later-phase work, use Chart 1 / S-64 style evidence as primary reference direction.
OpenCPN may be used as engineering comparison only.

## Anti-patterns

Do not:
- accept “non-zero pixels exist” as sufficient graphical proof
- rely only on console counters when the task is about what is visibly rendered
- mix parity-harness bookkeeping with normative behavior claims