## Docker / Devcontainer Notes

The current mainline is Windows-first because the official product stack depends on Qt 6 desktop tooling and Windows-oriented validation.

The existing `.devcontainer` assets may still help with general C++ tooling, but they are not the primary or fully supported path for validating the Qt Widgets host at this stage.

If you need reproducible Windows validation, prefer:

- Visual Studio with the Desktop C++ workload
- CMake + Ninja
- A matching Qt 6 desktop installation

For Linux container work, treat it as exploratory rather than release-signoff coverage until cross-platform Qt support is added back to the mainline.
