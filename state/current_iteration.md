# Current Iteration

- Task: `115-e400-chart1-csp-s64-verification-baseline`
- Status: `114-e400-alias-and-compatibility-audit` completed. The repository
  now has an explicit audited route for the non-direct e4.0.0 IDs without
  inventing unsupported official assets:
  - `VEHTRF01` stays inland-current
  - `BOYLAT52-56` / `BOYSPP50` stay legacy inland
  - `BCNCON81` / `DANGER53` / `BOYSPR02` / `BOYSPR03` stay residual
    compatibility
  - `BOYSPH79` / `TOPSHP33` stay alias/provenance audit only
- Blocker: none
- Previous task: `114-e400-alias-and-compatibility-audit` closed the route
  audit for the remaining non-direct e4.0.0 IDs without changing the active
  runtime path or widening the runtime public ABI.
