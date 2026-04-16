# AGENTS.md

## Project mission

Build a **DLL-first marine chart system** with:
- `chart_runtime.dll` as the primary product
- `chart_qtwidgets.dll` as the Qt Widgets integration layer
- `chart_standalone.exe` as the official demo host

The repository is a **reusable chart engine first**, not a standalone app first.

## Architecture rules

### 1. DLL-first is mandatory
Always preserve this product hierarchy:
1. `chart_runtime.dll`
2. `chart_qtwidgets.dll`
3. `chart_standalone.exe`

### 2. Qt Widgets is the shell; Qt 6 RHI is the main rendering backend
- Main UI framework: **Qt Widgets**
- Main rendering backend: **Qt 6 RHI**
- Do **not** switch the mainline to QML.
- Do **not** use QPainter as the main chart rendering path.

### 3. UI shell and runtime/render core must remain separate
`chart_standalone` and `chart_qtwidgets` may host the runtime, but must not absorb:
- chart parsing
- SENC build logic
- scene construction internals
- GPU resource ownership
- portrayal rules
- chart selection / quilt / zoom policy

### 4. Runtime public API must stay narrow
Public runtime API should prefer:
- C API
- opaque handles
- narrow DTO-style structs

Avoid exposing in runtime public ABI:
- QWidget
- QMainWindow
- QRhi
- parser internals
- deep scene/render classes

### 5. Phase roadmap
- **Phase 1**: S57 / CM93 / S-101 single-chart rendering through full SENC v1 pipeline.
- **Phase 2**: chart catalog / coverage / quilt / zoom.
- **Phase 3**: full nautical symbology.

### 6. One task at a time
Do not silently combine multiple task files into one large change unless explicitly requested.

### 7. Always preserve boundaries
Before implementing, explicitly identify what the current task will **not** modify.

### 8. Required outputs for each task
Update:
- `state/current_iteration.md`
- `state/done.md`
- `state/blocked.md` if blocked

### 9. Verification is mandatory
Do not stop at code edits. Run at least the minimum verification requested by the task.

### 10. Prohibited patterns
Do not:
- move core logic into `MainWindow`
- move parsing logic into `ChartViewWidget`
- expose QRhi or QWidget in the runtime C API
- couple readers directly to render code
- bypass SENC in Phase 1 flow
- implement Phase 2 or Phase 3 work inside Phase 1 tasks

### 11. One completed task = one git commit
After completing a task, and only after:
- the code changes for that task are finished,
- the required verification has actually been run and passed,
- `state/current_iteration.md` and `state/done.md` have been updated,
create exactly one git commit for that task.

Do not mix multiple tasks into one commit.
Do not commit a task as complete before verification.
If blocked, update `state/blocked.md` instead of committing it as finished.