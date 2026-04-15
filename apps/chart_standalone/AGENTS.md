# AGENTS.md — chart_standalone

## Layer mission
`chart_standalone` is the official demo/host shell.
It exists to:
- demonstrate how to host `chart_runtime.dll` through `chart_qtwidgets.dll`
- provide a usable standalone desktop shell
- act as a reference integration host

It is not the architectural center.

## Rules
- `MainWindow` must stay a host shell.
- Do not move chart parsing, SENC, scene building, render-core logic into this layer.
- Use the same integration path a normal Qt host would use where practical.
- Keep shell policy in the shell; do not leak it downward.
