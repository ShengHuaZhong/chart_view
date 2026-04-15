# Test Data Policy

## Local real chart data
Real S57 and CM93 chart files are provided locally and are not committed to the repository.

Use these cache variables or environment-backed settings:
- `CHARTSYS_ENABLE_REAL_CHART_TESTS`
- `CHARTSYS_S57_TESTDATA_ROOT`
- `CHARTSYS_CM93_TESTDATA_ROOT`

## Canonical fixtures
Each format must provide one canonical local fixture described by a manifest.

Expected manifest paths:
- `tests/data/s57/manifest.example.json`
- `tests/data/cm93/manifest.example.json`

## Phase 1 policy
- S57 real-data tests: required
- CM93 real-data tests: required
- S-101 real-data tests: optional until local sample data is provided

## Missing data behavior
If required S57 or CM93 local data is missing while real chart tests are enabled:
- tests must fail clearly
- tasks depending on those fixtures are not considered complete