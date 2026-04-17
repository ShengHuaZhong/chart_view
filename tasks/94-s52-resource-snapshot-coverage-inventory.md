# 94 - S-52 resource snapshot coverage inventory

## Objective
Generate a deterministic coverage inventory from the vendored `Release_5.14.0/s57data/chartsymbols.xml` snapshot so the remaining Phase 6B work has a single repo-owned progress baseline.

## Phase
- Phase 6B

## Layer
- portrayal
- tests
- docs

## Depends on
- 93a
- display-completeness follow-up after 93a

## In scope
- Add the Phase 6B task chain `94-99`
- Generate a deterministic resource-snapshot coverage inventory baseline
- Record parser/compiler supported, partial, and unsupported inventory status
- Record degraded rows with explicit reasons
- Add focused verification for deterministic inventory generation

## Out of scope
- No runtime public ABI changes
- No host changes
- No lookup/CSP/render behavior changes yet
- No `s52plib` integration

## Done when
The repository contains a committed, deterministic Phase 6B coverage inventory baseline for the vendored `chartsymbols.xml` snapshot, and that baseline is verified by an automated test.

## Verification
- Build the new inventory test target
- Run the focused inventory test target
- Generate/update the committed inventory reference from the same test path
