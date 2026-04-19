# Phase 6D Execution Workstreams

Phase 6D is now executed as four explicit workstreams instead of one over-broad
"all lookup-row closure" task.

## Reference note

- local PresLib reference missing:
  - `docs/reference_local/S-52_PresLib_e4.0.0_Part_I_Clean_Draft.pdf` was not
    present in the local working tree during this rewrite
- because of that, the current workstreams must continue from the repository's
  committed fallback-path evidence and internal runtime architecture only
- if the local e4.0.0 word-processed PresLib draft becomes available later, it
  may be used as a narrative reference for command syntax, lookup semantics, CSP
  naming, and non-lookup behavior checks
- this note does not change the repository's declared higher-edition normative
  target in `AGENTS.md`

## Workstream mapping

### Workstream A - IR compiler
- owner task:
  - `108-s52-all-lookuprow-closure`
- focus:
  - compiler-owned normalization, aliasing, and spillover cleanup
  - legacy token to canonical IR token mapping for currently explicit fallback-path
    inputs
  - closure of machine-readable input naming gaps without widening the runtime ABI
- non-goals:
  - no CSP behavior expansion
  - no host changes
  - no new manual overlay wave beyond the already completed task-107 / task-107a

### Workstream B - CSP engine
- owner task:
  - `109-s52-harness-and-standalone-proof`
- focus:
  - runtime-owned CSP opcode and behavior closure
  - depth / safety / sounding / obstruction / wreck / restriction / lights /
    newobj family behavior
  - clear separation between lookup-driven instructions and non-lookup free-text
    behavior
- non-goals:
  - no asset sweep
  - no host architecture changes
  - no runtime public ABI expansion

### Workstream C - asset completion
- owner tasks:
  - `107-s52-manual-overlay-asset-pack`
  - `107a-inland-current-symbols-vehtrf01`
- focus:
  - maritime confirmed manual overlay first:
    - `ARCSLN01`
    - `NEWOBJ01`
  - inland-current separate task:
    - `VEHTRF01`
  - explicit quarantine for:
    - legacy inland decision gate:
      - `BOYLAT52`
      - `BOYLAT53`
      - `BOYLAT54`
      - `BOYLAT55`
      - `BOYLAT56`
      - `BOYSPP50`
    - residual compatibility IDs:
      - `BCNCON81`
      - `DANGER53`
      - `BOYSPR02`
      - `BOYSPR03`
- non-goals:
  - do not silently reclassify inland / legacy inland / residual compatibility
    IDs as ordinary maritime S-52 closure

### Workstream D - verification baseline
- owner task:
  - `110-phase6d-all-lookuprow-verification`
- focus:
  - focused build / test / direct standalone host evidence
  - repository-owned graphical/reference evidence
  - explicit reporting split for:
    - ordinary maritime S57 rows
    - inland-current rows
    - legacy inland decision-gate rows
    - residual compatibility rows
    - internal/meta rows
- non-goals:
  - no ECDIS compliance claim
  - no OpenCPN-as-normative claim

## Task 106 status

`106-s52-upstream-supplemental-opencpn-assets` remains a provenance split /
route-closure description, not a continuing implementation workstream.

- it records that the audited supplemental OpenCPN route did not provide honest
  vendorable resource definitions for the originally mixed missing-ID set
- it does not own IR, CSP, asset, or verification closure by itself
- it should not be used to claim that the later workstreams are already complete

## Boundary reminders

- OpenCPN remains a fallback input / engineering reference only
- the runtime public ABI stays narrow and unchanged
- no workstream may move runtime logic into `chart_qtwidgets` or
  `chart_standalone`
- no workstream may use coverage-accounting tricks to hide ordinary S57 backlog
