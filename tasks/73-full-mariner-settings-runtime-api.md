# 73 - Full mariner settings runtime API

## Objective
Expose the Phase 5 S-52 mariner settings and rule-filter controls through a narrow runtime C API.

## Phase
- Phase 5

## Layer
- runtime
- portrayal
- verification

## Depends on
- 72

## In scope
- Add DTO-style mariner-settings C API
- Add object-class and rule-filter configuration surfaces
- Keep Qt / PROJ / renderer internals out of the public ABI

## Out of scope
- No host UI wiring yet
- No direct renderer refactor beyond what the API requires

## Inputs
- AGENTS.md
- docs/phase_roadmap.md
- tasks/72-complete-s52-lookup-and-rule-ir.md

## Required changes
- Extend public runtime types and functions narrowly
- Add runtime API tests for get/set/filter behavior

## Deliverables
- New public DTOs and C API entry points
- API verification coverage

## Done when
Hosts can configure Phase 5 S-52 mariner settings and filtering through the runtime API without taking ownership of rule logic.

## Verification
- Build runtime API tests
- Run focused mariner-settings / filter tests

## Notes
- Public ABI must stay narrow and stable.
