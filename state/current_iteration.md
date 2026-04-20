# Current Iteration

- Task: `115-e400-chart1-csp-s64-verification-baseline`
- Status: blocked. The current DAI-driven runtime path and repo-owned
  verification surfaces still work, but task `115` cannot honestly establish an
  official-input Chart 1 / colour test / CSP UML / S-64 verification baseline
  because the required official local packages are missing.
- Blocker:
  - missing Chart 1 pseudo-S57 package
  - missing colour test S-57 package
  - missing CSP UML / Enterprise Architect package
  - missing S-64 test data
  - missing higher-edition official references matching the repo-declared
    target edition
  - current repo-owned surrogate baselines also fail on the preferred path:
    - `runtime.s64_reference_smoke` currently fails with `SIGSEGV`
    - `runtime.chart1_s64_reference_harness` currently fails
- Previous task: `114-e400-alias-and-compatibility-audit` closed the route
  audit for the remaining non-direct e4.0.0 IDs without changing the active
  runtime path or widening the runtime public ABI.
