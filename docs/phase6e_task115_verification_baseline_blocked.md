# Phase 6E Task 115: Official Verification Baseline Blocked

Task `115-e400-chart1-csp-s64-verification-baseline` attempts to establish the
next official-input verification baseline for the e4.0.0 DAI-driven path.

This task is currently **blocked** because the repository does not yet have the
official local verification packages needed to turn the existing repo-owned
reference harnesses into an honest official-input baseline.

## Local official inputs currently present

- `docs/reference_local/PresLib_e4.0.0.dai`
- `docs/reference_local/S-52_PresLib_e4.0.0_Part_I_Clean_Draft.pdf`

These were enough for:

- official DAI ingest
- compiler/loader integration
- official static-asset wave 1
- alias / compatibility audit

They are **not** enough to claim the next Chart 1 / colour test / CSP UML /
S-64 official verification baseline.

## Missing official verification inputs

- Chart 1 pseudo-S57 package
- colour test S-57 package
- CSP UML / Enterprise Architect package
- S-64 test data
- higher-edition official references matching the repo-declared target edition

## Current fallback/repo-owned evidence that still works

The repository still has working non-official verification surfaces:

- `runtime.e400_dai_source_catalog_bridge`
- `runtime.s52_catalog_compiler`
- `runtime.s52_presentation_assets`
- `runtime.feature_symbolizer`
- `runtime.chart1_s64_reference_harness`
- `runtime.s64_reference_smoke`
- `qtwidgets.smoke`
- direct standalone real-S57 host smoke

These prove that the active runtime path is still healthy. They do **not**
replace the missing official inputs listed above.

## Additional observed failures on the current preferred path

While preparing the task-115 focused verification matrix, a narrow local test
support fix was needed so `chart1_s64_reference_harness_tests` would link
against the current e4.0.0 DAI bridge.

After that support fix, the focused matrix still showed two failing repo-owned
verification surfaces:

- `runtime.s64_reference_smoke`
  - currently fails with `SIGSEGV`
  - the failing test case is:
    - `S64-inspired reference smoke applies simplified buoy symbols and suppresses disabled soundings and labels`
- `runtime.chart1_s64_reference_harness`
  - currently fails on the current preferred path as well

So task 115 is blocked for two independent reasons:

1. missing official verification packages
2. current repo-owned Chart 1 / S-64 surrogate baselines are not yet stable on
   the present official preferred path

## Bottom line

Task 115 cannot be honestly marked complete until the missing official
verification inputs are present locally or the task scope is explicitly
redefined, and until the failing repo-owned surrogate baselines are stabilized.
