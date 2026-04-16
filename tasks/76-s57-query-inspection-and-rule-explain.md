# 76 - S57 query, inspection, and rule explain

## Objective
Expose runtime-owned feature query and rule-explanation surfaces for the richer Phase 5 S57 model.

## Phase
- Phase 5

## Layer
- runtime
- chart_data
- portrayal
- verification

## Depends on
- 75

## In scope
- Add runtime-owned feature query/describe support
- Surface enough rule-explain data for debugging and inspection
- Keep the public API DTO-based

## Out of scope
- No host UI implementation yet
- No plugin/query tooling outside the runtime boundary

## Inputs
- AGENTS.md
- docs/phase_roadmap.md
- tasks/75-full-s52-renderer-integration-s57.md

## Required changes
- Add internal query path and narrow public DTOs
- Add focused query/describe tests

## Deliverables
- Runtime query/inspection capability
- Tests for feature summaries and rule explanations

## Done when
Hosts can ask the runtime for feature summaries and active-rule explanations without direct access to parser or renderer internals.

## Verification
- Build query/inspection API tests
- Run focused query / explain coverage

## Notes
- Keep ownership of feature semantics inside `chart_runtime`.
