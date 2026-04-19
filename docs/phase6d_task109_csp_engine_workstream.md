# Phase 6D Task 109 - CSP Engine Workstream

## Scope

Task `109-s52-harness-and-standalone-proof` is the Phase 6D workstream-B closure
for the runtime-owned CSP engine. The goal is to move the active runtime path
closer to the current S-52 Presentation Library naming and behavior semantics
without widening the runtime ABI or moving logic into the host.

This task keeps the architectural boundary unchanged:

- machine-readable fallback input still comes from the pinned vendored
  OpenCPN-style `chartsymbols.xml` snapshot
- normative portrayal intent is still judged against IHO S-52 / Annex A
- `chart_view` continues to compile the fallback input into its own IR and apply
  CSP behavior inside `chart_runtime`

## Local PresLib reference

This workstream used the local, uncommitted word-processed reference:

- `docs/reference_local/S-52_PresLib_e4.0.0_Part_I_Clean_Draft.pdf`

The document was used only as a narrative behavior and naming reference. It is
not vendored as a runtime asset and does not change the repository's declared
final target edition.

## Legacy-to-canonical CSP naming boundary

The pinned fallback input still uses several legacy CSP tokens. Task 109 keeps
those raw tokens visible as machine-input provenance while normalizing them onto
the runtime's internal semantic opcode layer.

The currently covered alias set is:

- `LIGHTS05` -> canonical family token `LIGHTS06`
- `SYMINS01` -> canonical family token `SYMINS02`
- `SOUNDG02` -> canonical family token `SOUNDG03`
- `DEPARE02` -> canonical family token `DEPARE03`
- `SLCONS03` -> canonical family token `SLCONS04`
- `OBSTRN04` -> canonical family token `OBSTRN07`
- `RESARE02` -> canonical family token `RESARE04`
- `WRECKS02` -> canonical family token `WRECKS05`

Unchanged tokens that remain stable in the active runtime path include:

- `RESTRN01`
- `TOPMAR01`

## Lookup / CSP / free-text boundary

Task 109 keeps three layers distinct:

1. lookup-selected symbol instructions from the compiled catalog
2. CSP-owned behavior that appends or selects runtime instructions
3. free-text or non-lookup source text that remains source/input provenance

The active NEWOBJ fail-safe is the clearest example of this split:

- the fallback snapshot still contains `CS(SYMINS01)` rows for `NEWOBJ`
- task 109 does not claim the raw `SYMINS01` token is the final portrayal truth
- instead, the runtime-owned CSP layer interprets that conditional as a fail-safe
  request and appends the `NEWOBJ01` manual overlay instructions already closed
  by task `107`

That behavior stays internal to `chart_runtime` and does not widen the runtime
public ABI.

## Behavior closed by task 109

Task 109 closes these focused CSP behaviors on the active runtime path:

- legacy/canonical alias acceptance for the covered CSP token set
- continued lights handling through the `LIGHTS05` / `LIGHTS06` family
- continued sounding/depth handling through the `SOUNDG02` / `SOUNDG03` and
  `DEPARE02` / `DEPARE03` families
- runtime-owned `NEWOBJ` fail-safe closure:
  - point `NEWOBJ` + `SYMINS` -> `SY(NEWOBJ01)` fallback
  - line `NEWOBJ` + `SYMINS` -> `LC(NEWOBJ01)` fallback
  - area `NEWOBJ` + `SYMINS` -> `SY(NEWOBJ01)` plus dashed boundary fallback

## What task 109 does not claim

Task 109 does not claim:

- full CSP parity with the final higher-edition target
- full S-64 pass
- ECDIS compliance
- that all remaining obstruction / wreck / restriction families are complete
  beyond the covered alias and active-path closure noted above
