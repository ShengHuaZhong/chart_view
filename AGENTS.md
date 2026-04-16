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
- PROJ types
- parser internals
- deep scene/render classes
- font-engine internals

### 5. Phase roadmap
- **Phase 1**: S57 / CM93 / S-101 single-chart rendering through full SENC v1 pipeline.
- **Phase 2**: chart catalog / coverage / quilt / zoom.
- **Phase 3**: generic semantic portrayal baseline.
- **Phase 4**: projected quilting through PROJ, S-52-based S57 display, and Unicode / multilingual text.

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
- expose QRhi, QWidget, or PROJ handles in the runtime C API
- couple readers directly to render code
- bypass SENC in Phase 1 flow
- treat OpenCPN as the normative specification for portrayal behavior
- reduce Unicode / multilingual work to `wchar_t` plumbing only
- implement future-phase work inside earlier-phase tasks unless the task explicitly asks for it

### 11. One completed task = one git commit
After completing a task, and only after:
- the code changes for that task are finished,
- the required verification has actually been run and passed,
- `state/current_iteration.md` and `state/done.md` have been updated,
create exactly one git commit for that task.

Do not mix multiple tasks into one commit.
Do not commit a task as complete before verification.
If blocked, update `state/blocked.md` instead of committing it as finished.

### 12. Phase 4 projection / S-52 / Unicode rules
- Use **PROJ** inside `chart_runtime` for projection and coordinate-transform work. Keep PROJ ownership and handles private to the runtime.
- Projection solves coordinate transforms only. Coverage resolution, chart selection, patch clipping, seam handling, and quilt policy remain runtime responsibilities.
- For one frame / quilt plan, define one common display projection and transform chart inputs into that display space. Do **not** let each chart render in its own unrelated projection space.
- Treat **IHO S-52 / Annex A / S-64** as the normative source for Phase 4 portrayal behavior.
- OpenCPN may be used as an engineering reference for behavior and decomposition, but do **not** copy its code and do **not** treat it as the normative spec.
- Unicode / multilingual support means: Unicode-safe text handling, label selection policy, font fallback, and glyph caching. It is not complete when only ASCII or single-font rendering works.
- Keep Phase 4 narrowly centered on **S57 first**. Do not silently expand the first Phase 4 pass into full S-101 portrayal or unrelated ECDIS scope.
