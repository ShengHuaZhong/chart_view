# Coding Rules

## General
- Use modern C++20.
- Favor explicit ownership and narrow interfaces.
- Keep classes small and responsibility-focused.
- Avoid god objects.

## Naming
- Types: `PascalCase`
- Functions / methods: `camelCase`
- member fields: `m_foo`
- constants: `kFoo`
- enums: `enum class`

## Headers / source
- One primary class per pair of files when practical.
- Keep public headers minimal.
- Do not leak heavy internal dependencies into public headers.

## Runtime API
- Runtime ABI uses plain structs and opaque handles.
- No QWidget / QRhi / QMainWindow in runtime public headers.

## Qt Widgets layer
- `ChartViewWidget` is a host container only.
- Do not parse chart files in QWidget code.
- Do not create chart policy logic in QWidget code.

## Rendering
- Main rendering path is Qt 6 RHI.
- Do not use QPainter as the main chart rendering solution.
- Do not mix parsing with render submission.

## Tasks and change discipline
- One task at a time.
- State boundaries before editing.
- Add verification notes after each task.
