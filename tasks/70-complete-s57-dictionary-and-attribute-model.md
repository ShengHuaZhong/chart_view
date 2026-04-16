# 70 - Complete S57 dictionary and attribute model

## Objective
Upgrade the internal S57 semantic model from the current narrow baseline to a portrayal/query/update-ready dictionary and attribute model.

## Phase
- Phase 5

## Layer
- runtime
- chart_data
- verification

## Depends on
- 69

## In scope
- Add broader object-class and attribute acronym coverage
- Preserve multi-value and national attributes in the internal source/domain model
- Prepare data needed by full S-52 portrayal and inspection

## Out of scope
- No public query API yet
- No host/UI changes
- No renderer rule execution changes yet

## Inputs
- AGENTS.md
- docs/phase_roadmap.md
- tasks/69-senc-v2-semantic-and-update-format.md

## Required changes
- Extend the S57 mapping/model layer
- Add tests for richer semantic retention and decoding

## Deliverables
- Expanded S57 dictionary coverage
- Attribute-model tests

## Done when
The runtime retains the S57 semantics needed by full Phase 5 portrayal and later inspection work without relying on `OBJ<code>` / `A<code>` as the normal path.

## Verification
- Build focused S57 semantic-retention tests
- Run the new dictionary / attribute-model coverage

## Notes
- Do not broaden into S-101 or CM93 in this task.
