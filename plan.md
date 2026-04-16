# plan.md

## 项目目标

构建一个 **DLL-first** 的 Qt 原生海图系统，面向集成、可验证迭代与长期演进。

核心产物：
- `chart_runtime.dll`：海图引擎主产品
- `chart_qtwidgets.dll`：Qt Widgets 集成层
- `chart_standalone.exe`：官方示例宿主

固定技术路线：
- UI 壳：Qt Widgets
- 主渲染：Qt 6 RHI
- `chart_runtime` 是主产品；`chart_qtwidgets` 是集成层；`chart_standalone` 只是宿主
- UI 壳与 runtime / render core 严格分离
- 以任务驱动方式推进，每次只完成一个 task
- 如果缺少依赖库，优先使用使用vcpkg安装

---

## 总体架构

系统分为三层：

### 1. `chart_runtime.dll`
负责：
- S57 / CM93 / S-101 数据读取与规范化
- SENC v1 构建、校验、读取
- 统一内部要素模型
- Viewport / Projection / Scene / SceneSnapshot
- Coverage / ChartCatalog / QuiltPlanner / ZoomPolicy
- Portrayal / Symbolization / Label system
- RHI 渲染主线
- Pick / Query / Runtime-owned frame output

### 2. `chart_qtwidgets.dll`
负责：
- `ChartViewWidget`
- Qt 事件到 runtime 的桥接
- QWidget 承载
- resize / repaint / viewport 同步
- 只做 host / container，不吸收 runtime 核心职责

### 3. `chart_standalone.exe`
负责：
- `QMainWindow`
- 菜单、工具栏、状态栏、面板
- 打开图表命令
- 官方演示宿主

约束：
- `chart_runtime.dll` 是主产品
- `chart_qtwidgets.dll` 是集成层
- `chart_standalone.exe` 只是宿主，不是架构中心
- 公共 ABI 保持窄 C API / opaque handle / DTO 风格

---

## 四阶段路线

### 第一阶段：单张图 + 完整 SENC v1 闭环

#### 阶段目标
完成 S57、CM93、S-101 单张海图的完整闭环：

`源文件 -> Reader -> Normalizer -> FeatureChartDataset -> SENC v1 -> SceneBuilder -> Runtime Render -> Qt Host Display`

#### 阶段必须完成
1. DLL-first 工程骨架
2. 运行时 API 基础生命周期与渲染接口
3. 统一内部要素模型
4. 完整 SENC v1
5. 三种格式单图接通

#### 本阶段明确不做
- 多图拼接
- 连续缩放策略
- seam 处理
- S-52
- 多语言字体系统
- 插件

#### 阶段完成标准
- 三种格式各至少一张样例图能够完成：读取、规范化、SENC 构建/回读、Scene 构建、Runtime 渲染、Qt 宿主显示
- 显示结果非空白
- 最小 smoke test 通过

---

### 第二阶段：海图拼接 + 海图缩放

#### 阶段目标
在 SENC v1 基础上，实现多海图自动选择、拼接和缩放：

`多张 SENC -> ChartCatalog -> CoverageIndex -> ChartSelectionPolicy -> QuiltPlan -> MultiChart SceneSnapshot -> Render -> Zoom Replan`

#### 阶段必须完成
1. `ChartCatalog`
2. `CoverageIndex`
3. `ChartSelectionPolicy`
4. `QuiltPlan / QuiltPlanner`
5. `MultiChart SceneBuilder`
6. `ZoomPolicy`
7. Qt 缩放交互与 quilt 刷新

#### 本阶段明确不做
- 投影后的 seam 精修
- S-52 完整符号化
- 高级文本避让
- 多语言字体系统
- 插件与高级编辑

#### 阶段完成标准
- S57 / CM93 / S-101 各至少一组多图样例
- 都能完成目录扫描、Coverage 查询、QuiltPlan 生成、多图场景渲染、缩放重算与刷新
- 选图逻辑稳定，日志可输出 quilt 摘要

---

### 第三阶段：通用语义化表达基线

#### 阶段目标
在前两阶段稳定基础上，完成 **generic semantic portrayal baseline**，让输出从“几何可见”提升到“航海语义可读”。

#### 阶段必须完成
1. `PortrayalRegistry`
2. `FeatureSymbolizer`
3. `DisplayPriorityModel`
4. point / line / area / text 的语义化 symbol renderers
5. S57 / S-101 的 baseline rule tables
6. symbolized render smoke 与阶段验证文档

#### 本阶段明确不做
- 基于 S-52 的完整标准对齐
- PROJ 投影体系
- 字体回退 / glyph cache / 多语言名称选择终版
- 完整 ECDIS 级 label engine

#### 阶段完成标准
- S57 / S-101 关键对象能够正确基础符号化、正确分层、正确基础标注
- Phase 3 结果是 runtime-owned semantic portrayal baseline，而不是完整生产级 portrayal catalogue

---

### 第四阶段：投影 + 基于 S-52 的 S57 完整渲染 + Unicode / 多语言文本

#### 阶段目标
在前三阶段基线上，建立：
- 使用 **PROJ** 的 runtime 内部投影体系
- 投影后的 coverage / quilt / patch clipping / seam handling
- **基于 S-52** 的 S57 完整渲染基线
- Unicode 文本、字体回退、glyph cache 与多语言名称选择

阶段闭环目标：

`S57/SENC -> ProjectionContext(PROJ) -> Projected Coverage/Quilt -> S-52 Lookup & Conditional Symbology -> Symbol Instruction Render -> Unicode Label Layout -> Runtime Render -> Qt Host Display`

#### 阶段必须完成
1. PROJ-backed `ProjectionContext`
2. projected scene / coverage / quilt patch clipping
3. projected quilt real-chart smoke
4. S-52 presentation assets / lookup / display settings / conditional symbology
5. 现有 renderers 升级为执行 S-52 instruction
6. Unicode 文本主线
7. 字体回退与 glyph cache
8. 多语言名称选择与投影后标签布局
9. 集成 real-chart smoke 与 Phase 4 verification

#### 本阶段明确不做
- 完整 ECDIS compliance 声称
- 全量 S-100 产品族扩展
- 插件系统
- Route/AIS 高级交互
- 非必要的 UI 美化
- 首轮 Phase 4 中把 S-101 portrayal 与 S57 同级推进

#### 阶段完成标准
- `chart_runtime` 内部完成统一投影主线，projection 不再是 renderer 里的分散近似公式
- quilt patch 与 seam handling 建立在统一 display projection 上
- S57 真实海图通过 projected quilt + S-52 + Unicode label 的 integrated smoke
- 基本 day/dusk/night、display settings、关键 conditional symbology 与多语言标签可验证
- 形成 Phase 4 verification 文档，明确 achieved baseline 与 remaining non-compliance scope

---

## 当前仓库状态（按最新状态文件口径）

- Phase 1：已完成
- Phase 2：已完成
- Phase 3：已完成，结果为 **generic semantic portrayal baseline**
- Phase 4：尚未开始，需要从 `tasks/53-66` 开始推进

---

## Phase 4 推荐任务顺序

1. `53-proj-projection-context-core`
2. `54-projected-scene-and-coverage-space`
3. `55-projected-quilt-seams-and-patch-clipping`
4. `56-projected-s57-real-chart-smoke`
5. `57-s52-presentation-assets-adapter`
6. `58-s52-lookup-and-instruction-model`
7. `59-s52-display-settings-and-conditional-symbology`
8. `60-s52-renderer-integration-s57`
9. `61-unicode-text-system-core`
10. `62-font-fallback-and-glyph-cache`
11. `63-multilingual-label-selection-and-projected-layout`
12. `64-s52-unicode-real-chart-smoke-s57`
13. `65-s64-reference-behavior-smoke`
14. `66-phase4-demo-verification`

---

## 任务执行原则

### 1. 一次只做一个 task
AI 每次只执行一个 `tasks/*.md` 文件，不静默合并多个任务。

### 2. 不跳过依赖
只做依赖已满足的任务。

### 3. 每轮必须先说明边界
每轮执行前，必须先说明：
- 本任务属于哪一层
- 不会跨哪些边界
- 最小实现方案是什么

### 4. 每轮必须验证
至少做任务要求的：
- build
- smoke test
- 运行验证

### 5. 每轮必须回写状态
更新：
- `state/current_iteration.md`
- `state/done.md`
- 若阻塞则更新 `state/blocked.md`

### 6. 每完成一个 task，提交一个 git commit
仅在以下条件全部满足后提交：
- 当前 task 代码改动完成
- 该 task 要求的验证已经实际运行并通过
- `state/current_iteration.md` 与 `state/done.md` 已更新

禁止：
- 多个 task 共用一个 commit
- 未验证通过就把 task 作为完成提交
- 被 blocker 卡住时伪装成已完成提交

---

## Phase 4 额外规则

- PROJ 只负责坐标与投影变换，不负责 coverage 决策、chart selection、patch clipping、seam 规则或 quilt policy。
- 每一帧 / 每一个 quilt plan 必须先确定一个统一 display projection，再把候选 chart 投入同一平面空间。
- S-52 / Annex A / S-64 是 Phase 4 表现行为的规范真相源。
- OpenCPN 只能作为工程参考和行为对照，不能作为规范真相源，也不能直接搬运代码。
- “宽字符支持”不是 Phase 4 文本目标。真正目标是：Unicode-safe 文本通道、字体回退、glyph cache、多语言名称选择与可验证标签显示。
- Phase 4 首轮聚焦 **S57 first**，不要把范围静默扩展到完整 S-101 portrayal 或其他 ECDIS 范围。

---

## 项目文件约束

### 架构约束
- `AGENTS.md`
- `docs/phase_roadmap.md`

### 代码风格约束
- `docs/coding_rules.md`
- `.clang-format`
- `.clang-tidy`

### 构建环境约束
- `docs/build_environment.md`
- `CMakePresets.json`

---

## 成功标准

整个项目成功，不是因为“能打开一张图”，而是因为：
- `chart_runtime.dll` 成为可复用核心引擎
- `chart_qtwidgets.dll` 成为稳定集成层
- `chart_standalone.exe` 只是示例宿主
- SENC 成为统一缓存主线
- Phase 1 打通单图
- Phase 2 打通拼接与缩放
- Phase 3 建立语义化表达基线
- Phase 4 建立投影、S-52 与 Unicode / 多语言文本主线

---

## 下一步建议

优先从以下文件开始落地：
1. `AGENTS.md`
2. `plan.md`
3. `docs/phase_roadmap.md`
4. `tasks/53-proj-projection-context-core.md`
5. `tasks/54-projected-scene-and-coverage-space.md`
6. `tasks/55-projected-quilt-seams-and-patch-clipping.md`
