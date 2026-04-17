# OpenCPN `s57data` snapshot notice

This directory vendors a fixed snapshot of OpenCPN `data/s57data` resources for the approved Phase 6A fallback path.

- Upstream repository: `https://github.com/OpenCPN/OpenCPN.git`
- Upstream ref: `Release_5.14.0`
- Upstream commit: `91f3b674366068a6ecd61a5e9aba204bba85f57e`
- Snapshot date: `2026-04-17`

Use in this repository:

- The files under `s57data/` are treated as **read-only compiler inputs**.
- `chart_view` compiles these resources into its own source/compiled catalog and instruction IR.
- `chart_runtime` does **not** link or embed OpenCPN `s52plib`.
- Normative portrayal truth remains IHO `S-52 / Annex A / S-64`.

License reference:

- See `COPYING.gplv2` in this directory.
- Upstream reference: `https://github.com/OpenCPN/OpenCPN/blob/Release_5.14.0/COPYING.gplv2`
