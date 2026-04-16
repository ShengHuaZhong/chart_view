# Test Data

This directory contains manifest templates for local chart test data.
See `docs/test_data_policy.md` for the full policy.

## Setup

1. Copy the `manifest.example.json` files to your local test-data roots.
2. Rename them to `manifest.json`.
3. Edit the chart entries to match your local files.
4. Set the CMake variables (`CHARTSYS_S57_TESTDATA_ROOT`, etc.) to point to your data.
5. Enable real chart tests: `CHARTSYS_ENABLE_REAL_CHART_TESTS=ON`.

## Checked-in Smoke Fixture

- `s101/smoke_dataset.101` is a synthetic Phase-1 host smoke fixture used by open-chart smoke tests.
- It does not represent real S-101 content; it encodes a tiny renderable geometry set so the full runtime -> Qt host presentation path can be verified without real S-101 data.
