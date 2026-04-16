# Current Iteration

- Task: `81-phase5-demo-verification`
- Status: `80-phase5-host-binding-and-demo-controls completed; the repository now has Qt host bindings for Phase 5 mariner settings, object-class filters, and stable rule filters, plus focused host smoke coverage that exercises the runtime-owned controls path through chart_qtwidgets and chart_standalone`
- Blocker: `none active`
- Previous task: `80-phase5-host-binding-and-demo-controls` completed by adding Phase 5 control bridge methods to `ChartViewWidget` / `RuntimeBridge`, wiring a dedicated controls dock into `MainWindow`, adding `qtwidgets.smoke` and `chart_standalone.phase5_controls.smoke` coverage, and hardening `TextLabelRenderer` glyph-cache lifetime so host smoke teardown completes cleanly without widening into task-81 Phase 5 closeout work.
