# Task 11a — Chart Test Data Fixtures

## Objective
Prepare canonical local test-data conventions for real S57 and CM93 integration tests.

## Layer
verification / testdata / chart_data

## In scope
- define local test-data root conventions
- define how S57 fixtures are discovered
- define how CM93 fixtures are discovered
- define required manifest files
- define failure policy when required local data is missing
- add build/test configuration variables for local chart data

## Out of scope
- no parser implementation
- no SENC implementation
- no renderer implementation

## Required changes
- add `docs/test_data_policy.md`
- add `tests/data/README.md`
- add manifest templates:
  - `tests/data/s57/manifest.example.json`
  - `tests/data/cm93/manifest.example.json`
- add CMake cache variables:
  - `CHARTSYS_ENABLE_REAL_CHART_TESTS`
  - `CHARTSYS_S57_TESTDATA_ROOT`
  - `CHARTSYS_CM93_TESTDATA_ROOT`

## Done when
- repository has a clear local test-data policy
- S57 and CM93 tasks can refer to canonical local fixture roots
- missing required local data fails clearly instead of silently skipping