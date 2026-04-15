# plan.md

## 项目目标

构建一个 **DLL-first** 的 Qt 原生海图系统，面向集成与长期演进。

核心产物：

- `chart_runtime.dll`：海图引擎主产品
- `chart_qtwidgets.dll`：Qt Widgets 集成层
- `chart_standalone.exe`：官方示例宿主

固定技术路线：

- UI 壳：Qt Widgets
- 主渲染：Qt 6 RHI
- UI 壳与 runtime / render core 严格分离
- 以任务驱动的方式，让 AI 逐步自动迭代开发

---

## 总体架构

系统分为三层：

### 1. chart_runtime.dll
负责：

- S57 / CM93 / S-101 数据读取与规范化
- SENC v1 构建、校验、读取
- 统一内部要素模型
- Viewport / Scene / SceneSnapshot
- RHI 渲染主线
- ChartCatalog / CoverageIndex / QuiltPlanner
- ZoomPolicy
- Portrayal / Symbolization
- Pick / Query

### 2. chart_qtwidgets.dll
负责：

- `ChartViewWidget`
- Qt 事件到 runtime 的桥接
- QWidget 承载
- resize / repaint / viewport 同步

### 3. chart_standalone.exe
负责：

- `QMainWindow`
- 菜单、工具栏、状态栏、面板
- 打开图表命令
- 官方演示宿主

约束：

- `chart_runtime.dll` 是主产品
- `chart_qtwidgets.dll` 是集成层
- `chart_standalone.exe` 只是宿主，不是架构中心

---

## 三阶段路线

---

## 第一阶段：单张图 + 完整 SENC v1 闭环

### 阶段目标

完成 S57、CM93、S-101 单张海图的完整闭环：

`源文件 -> Reader -> Normalizer -> FeatureChartDataset -> SENC v1 -> SceneBuilder -> Runtime Render -> Qt Host Display`

### 本阶段必须完成

#### 1. DLL-first 工程骨架
- `chart_runtime.dll`
- `chart_qtwidgets.dll`
- `chart_standalone.exe`
- `CMakePresets.json`
- `vcpkg.json`
- install/export/package config

#### 2. 运行时 API
至少具备：
- create runtime
- initialize runtime
- shutdown runtime
- open/build/load chart/SENC
- set viewport
- render frame

#### 3. 统一内部要素模型
用于统一：
- S57
- CM93
- S-101

至少包含：
- point / line / area
- feature id
- class code
- 基础属性
- chart extent
- dataset metadata

#### 4. 完整 SENC v1
SENC v1 至少包含：
- FileHeader
- SourceManifest
- DatasetMeta
- FeatureTable
- GeometryBlob
- AttributeBlob
- SpatialIndex
- RenderCache
- PickIndex
- StringTable

#### 5. 三种格式单图接通
- S57 单图 -> SENC -> 渲染
- CM93 单图 -> SENC -> 渲染
- S-101 单图 -> SENC -> 渲染

### 本阶段明确不做
- 多图拼接
- 连续缩放策略
- MBTiles
- S-102
- 完整航海符号
- 插件
- 高级编辑
- UI 美化

### 阶段完成标准
- 三种格式各至少一张样例图
- 都能完成：
  - 读取
  - 规范化
  - SENC 构建/回读
  - Scene 构建
  - Runtime 渲染
  - Qt 宿主显示
- `chart_runtime.dll` 与 `chart_qtwidgets.dll` 可被独立链接
- 显示结果非空白
- 最小 smoke test 通过

---

## 第二阶段：海图拼接 + 海图缩放

### 阶段目标

在 SENC v1 基础上，实现多海图自动选择、拼接和缩放。

闭环目标：

`多张 SENC -> ChartCatalog -> CoverageIndex -> ChartSelectionPolicy -> QuiltPlan -> MultiChart SceneSnapshot -> Render -> Zoom Replan`

### 本阶段必须完成

#### 1. ChartCatalog
管理多张图的轻量元数据：
- chart id
- extent
- native scale
- usage band
- source type
- senc path
- validity state

#### 2. CoverageIndex
支持：
- 按 viewport bbox 查询候选图
- 不使用全表线性扫

#### 3. ChartSelectionPolicy
定义：
- scale 匹配规则
- usage band / priority 规则
- 基本重叠图选择逻辑

#### 4. QuiltPlan / QuiltPlanner
输出当前帧应显示哪些图，以及显示顺序。

#### 5. MultiChart SceneBuilder
把单图 scene builder 升级为多图 scene builder。

#### 6. ZoomPolicy
实现：
- scale step
- overzoom / underzoom
- 缩放导致的重算时机

#### 7. Qt 缩放交互
- 滚轮缩放
- 锚点缩放
- viewport 更新
- quilt plan 重算
- render 刷新

### 本阶段明确不做
- seam 美化
- 高级文本避让
- 完整航海符号
- S-102
- 插件
- Route/AIS 高级编辑

### 阶段完成标准
- S57 / CM93 / S-101 各至少一组多图样例
- 都能完成：
  - 多图目录扫描
  - ChartCatalog 构建
  - Coverage 查询
  - QuiltPlan 生成
  - 多图场景渲染
  - 缩放重算与刷新
- 缩放过程中不崩溃
- 选图逻辑稳定
- 日志可输出当前 scale、候选图数量、选中图数量、QuiltPlan 摘要

---

## 第三阶段：完整航海符号

### 阶段目标

在前两阶段稳定基础上，完成完整航海符号系统。

重点对象：
- S57
- S-101

CM93 优先复用兼容映射，不单独设计全新符号体系。

### 本阶段必须完成

#### 1. PortrayalRegistry
定义：
- SymbolRule
- LineStyleRule
- AreaFillRule
- TextRule

#### 2. FeatureSymbolizer
将：
- class code
- attributes
- context

映射到：
- style key
- display priority
- layer group
- symbol selection

#### 3. Display priority / layering
建立完整绘制顺序和分层规则。

#### 4. Point / Line / Area symbolization
完成：
- 点符号
- 线型
- 面填充
- 基础专题对象渲染

#### 5. Text / Label 基础主线
完成最小文本标注能力。

### 本阶段明确不做
- S-102
- 完整 S-100 全家桶
- 高级 label engine 终版
- 插件系统
- 最终 UI 美化

### 阶段完成标准
- S57 / S-101 的关键对象能够：
  - 正确符号化
  - 正确分层
  - 正确基础标注
- 与第二阶段相比，视觉效果从“几何可见”升级为“航海语义可读”

---

## 任务执行原则

### 1. 一次只做一个 task
AI 每次只执行一个 `tasks/*.md` 文件。

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

---

## 项目文件约束

### 架构约束
- 架构边界：`AGENTS.md`
- 子模块约束：
  - `chart_runtime/AGENTS.md`
  - `chart_qtwidgets/AGENTS.md`
  - `chart_standalone/AGENTS.md`

### 代码风格约束
- `docs/coding_rules.md`
- `.clang-format`
- `.clang-tidy`
- `.editorconfig`

### 构建环境约束
- `docs/build_environment.md`
- `CMakePresets.json`
- `vcpkg.json`

---

## 风险控制

### 第一类风险：单体化回潮
表现：
- MainWindow 成为总控
- ChartViewWidget 持有业务逻辑
- runtime API 被 UI 类型污染

控制方式：
- 强制分层
- 任务按层执行
- 在 `AGENTS.md` 中禁止跨层

### 第二类风险：阶段越界
表现：
- 第一阶段开始做 quilting
- 第二阶段开始做完整符号
- 第三阶段又回头补基础渲染

控制方式：
- 每阶段明确 Out of scope
- task 文件必须有 Done when

### 第三类风险：AI 任务发散
表现：
- 一个任务做成大重构
- 不更新状态文件
- 修改与任务无关模块

控制方式：
- 一次只做一个 task
- 阻塞时强制拆任务
- 更新 state 文件

---

## 当前推荐执行顺序

### Phase 1
先跑：
- `00` 到 `26`

### Phase 2
再跑：
- `27` 到 `40`

### Phase 3
最后跑：
- `41` 到 `52`

---

## 成功标准

整个项目成功，不是因为“能打开一张图”，而是因为：

- `chart_runtime.dll` 成为可复用核心引擎
- `chart_qtwidgets.dll` 成为稳定集成层
- `chart_standalone.exe` 只是示例宿主
- SENC 成为统一缓存主线
- 第一阶段打通单图
- 第二阶段打通拼接与缩放
- 第三阶段完成完整航海符号
- 后续还能继续扩展：
  - MBTiles
  - S-102
  - 更多 S-100 产品
  - 插件或脚本系统

---

## 下一步建议

优先从以下文件开始落地：

1. `AGENTS.md`
2. `docs/architecture.md`
3. `docs/build_environment.md`
4. `tasks/00-repo-bootstrap.md`
5. `tasks/01-runtime-api-contract.md`
6. `tasks/02-runtime-dll-skeleton.md`

如果仓库已经存在模板工程，先完成：
- 顶层 CMake 改造
- `chart_runtime` / `chart_qtwidgets` / `chart_standalone` 三层拆分
- package/export 可用
