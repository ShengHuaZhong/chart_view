# AGENTS.md — chart_qtwidgets

## Layer mission
`chart_qtwidgets` is the Qt Widgets integration layer for the chart runtime.
It exists to:
- host runtime rendering in QWidget-based applications
- translate Qt input and resize events into runtime operations
- provide `ChartViewWidget` and small Qt-facing bridge classes

It is not the chart engine.

## Ownership rules
This layer owns:
- `ChartViewWidget`
- Qt input event translation
- viewport-to-runtime synchronization
- paint/update scheduling hooks from QWidget side

This layer must not own:
- chart readers
- SENC build policy
- chart selection / quilt / zoom policy
- portrayal rules
- core scene construction logic

## Rules
- Widget is a host container, not the engine.
- Do not bypass runtime.
- Keep Qt-facing interfaces narrow.
- Do not absorb app-shell responsibilities.
