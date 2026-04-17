# 90 - S-52 display modes and mariner settings complete

## Objective
Complete the runtime-owned mariner settings and display-mode surface for the Phase 6 S-52 engine.

## Phase
- Phase 6

## Layer
- runtime API
- runtime portrayal

## Depends on
- 89

## In scope
- Complete palette, display category, point-symbol mode, boundary mode, and related mariner controls
- Wire the settings through the official Phase 6 portrayal path
- Keep existing class/rule filters intact

## Out of scope
- No new host UI phase
- No OpenCPN harness work yet
- No compliance claim work

## Inputs
- existing `chart_view_s52_mariner_settings_t`
- existing class/rule filter APIs
- completed Phase 6 engines from tasks 84-89

## Required changes
- Expand the existing runtime settings DTO/API as needed
- Thread the settings through the active portrayal path
- Add focused API and behavior regression tests

## Deliverables
- Completed runtime mariner-settings surface
- Regression coverage for settings behavior

## Done when
The runtime API can control the major Phase 6 display modes and mariner settings through the official portrayal path without introducing a parallel configuration system.

## Verification
- Build runtime API and portrayal tests
- Run focused mariner-settings regression coverage

## Notes
- Keep the ABI narrow and avoid exposing any renderer or asset internals.
