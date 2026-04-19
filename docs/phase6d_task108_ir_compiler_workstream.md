# Phase 6D Task 108: IR Compiler Normalization Workstream

Task 108 closes the first unblocked Phase 6D workstream: compiler-owned
normalization and alias cleanup on the fallback `chartsymbols.xml` input path.

## Reference note

- local PresLib reference missing:
  - `docs/reference_local/S-52_PresLib_e4.0.0_Part_I_Clean_Draft.pdf` was not
    present in the local working tree during task 108
- task 108 therefore uses the repository's committed fallback-path evidence and
  runtime-owned architecture as the implementation basis
- this work does not claim final higher-edition normative closure

## What task 108 normalized

Task 108 keeps `rawInstruction` text intact while normalizing compiled IR asset
tokens before lookup rows enter the runtime-owned catalog.

The covered compiler-owned spillover / alias set is:

- `DGPS01DRFSTA01` -> `RDOSTA02`
- `TOPSHP73TESOBJNAM...` -> `TOPSHP73`
- `TOPSHP09TESOBJNAM...` -> `TOPSHP09`
- `TOPSHP15TESOBJNAM...` -> `TOPSHP15`
- `TOPSHP81TESOBJNAM...` -> `TOPSHP81`
- `TOPSHP89TESOBJNAM...` -> `TOPSHP89`
- `TOPSHPT8TESOBJNAM...` -> `TOPSHPT8`
- `TOWERS74TXOBJNAM...` -> `TOWERS74`

## Boundary preserved

- no runtime public ABI changes
- no host changes
- no CSP behavior expansion
- no new manual overlays beyond tasks `107` / `107a`
- no coverage-accounting tricks

## Evidence

The focused compiler/inventory regressions now prove that the above spillover
tokens no longer remain partial merely because of compiler-side naming noise.

Task 108 does not close the remaining inland-current, legacy inland, residual
compatibility, or internal/meta buckets. Those remain explicit in the Phase 6D
workstream split.
