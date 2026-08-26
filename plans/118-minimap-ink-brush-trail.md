# Plan 118 - 程序 - 小地图经典墨水笔轨迹

## 协调

- Planner 负责人：Codex（Gavyn-side AI）。
- Executor 负责人：Codex（Gavyn-side AI）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@f8c5b6c64207efb4f4a5ce4cc6e3197585b266ac`。
- 本地实现方式（可选，仅作交接说明）：`feat/minimap-ink-brush-trail`；`C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan118-minimap-ink-brush`。
- 依赖 / 阻塞：依赖 Plan102 已发布的小地图坐标投影、回响路径视图与头像绘制；运行 UE Editor/命令前遵守同克隆 Unreal 锁。
- Writes:
  - `plans/118-minimap-ink-brush-trail.md`
  - `Source/ReEcho/{Public,Private}/UI/ReEchoMinimapCanvasWidget.*`
  - `Source/ReEcho/Private/Tests/ReEchoMinimapTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoCombatHudTests.cpp`
  - `Content/SourceArt/UI/CombatHud/MinimapInkBrush/`
  - `Content/ReEcho/Textures/UI/CombatHud/MinimapInkBrush/`
  - `Content/ReEcho/Materials/UI/M_UI_MinimapInkTrail.uasset`
  - `scripts/ue/import_minimap_ink_brush_assets.py`
  - `scripts/ue/author_minimap_ink_trail_material.py`
  - `Design/UI/ReEcho_UI修改指导.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - 精选 Win64 Editor 预构建包及 manifest（仅最终发布门禁刷新）。
- Stable Reads:
  - `Source/ReEcho/{Public,Private}/UI/ReEchoEncounterHudWidget.*`
  - `Content/ReEcho/UI/WBP_ReEchoEncounterHud.uasset`
  - `Config/DefaultGame.ini`
  - 外部交付目录 `正式-UI视觉/正式-UI视觉/战斗场景/经典墨水笔_Photoshop交付/`
- 影响模式：`SharedContract`。不改变小地图视图数据、录制或玩法权威，但扩展 `UReEchoMinimapCanvasWidget` 的 WBP 可调表现参数，并新增 Cook 可追踪的纹理/材质依赖。
- 兼容承诺 / 下游操作：Player/Echo 头像、坐标投影、轨迹颜色、透明底板和回响路径数据保持不变；墨水材质缺失时安全降级到现有纯色折线。策划/UI 可在 Encounter HUD 内的小地图控件 Details 调整笔触尺寸、间距、旋转/透明度抖动与 Grain 强度。
- 明确排除：不修改录制采样率、路径点数量/简化算法、Echo 移动/回放、头像资源、小地图边框或底板；不尝试让 UE 直接加载 Photoshop `.abr`，也不把 Photoshop/Procreate 动态引擎引入运行时。

## 锁定目标

用用户交付的“经典墨水笔”笔尖与 Grain 资产替换小地图内部现有 2px 纯色回响轨迹线。轨迹仍沿现有权威路径点和现有 Echo 颜色绘制，但视觉上改为连续、带不规则边缘与纸张颗粒缺口的墨水笔触；当前 Player/Echo 图标和所有小地图坐标语义保持不变。

UE 运行时采用沿折线确定性连续盖印透明笔尖的方式还原 Photoshop 笔刷：笔尖跟随路径方向，并按交付参数提供约 `18%` 的角度变化和 `16%` 的不透明度变化；Grain 由 UI 材质在每个笔尖内调制 Alpha。盖印必须跨路径段保持等距、同一轨迹在相同数据下每帧稳定，且具备单条轨迹最大盖印数，避免长录制路径导致 Slate 绘制无界增长。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoUI`、`AREA-UI`、`AREA-Tests`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `MOD-ReEchoUI.md`，均已加入 `Writes`。
- 设计意图：保持小地图数据与表现分离；GameMode 继续只提供世界路径，Slate Widget 只把路径投影后转换为可丢弃笔触盖印，不从资产或材质反向影响录制/回放。
- 权威状态与依赖：不新增玩法权威，不修改 `FReEchoMinimapView/FReEchoMinimapEchoEntry` 的数据来源。`UReEchoMinimapCanvasWidget` 拥有仅表现用的 Editor 参数与材质引用，`SReEchoMinimapCanvas` 只持有当前绘制快照和弱表现资源；依赖方向仍为主模块 UI → Slate/Engine 资产。
- 决策记录：
  - `.abr` 是 Photoshop 笔刷容器，UE 不具备运行时加载器；生产运行时只导入已交付的 `512×512` 透明笔尖与 `900×900` Grain，完整 `.abr`、参数 JSON/TXT 和 256px 备选笔尖归档在 SourceArt 供追溯。
  - 不把 Grain 离线烘进新 PNG，避免创建第三份不透明美术真源；UI 材质直接组合两张交付纹理，并乘入 Slate Vertex Color 保留每个 Echo 的既有颜色。
  - 使用确定性盖印而非单纯增粗 `MakeLines`，才能保留交付笔尖轮廓、方向和不透明度动态；使用最大盖印数限制成本，材质缺失时回退旧折线。
  - WBP 可调参数位于现有 `UReEchoMinimapCanvasWidget` 实例，不新增第二个小地图 WBP 或 Gameplay 状态。
- 相关文档同步范围：审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `README.md`；预期模块拓扑/索引不变。更新 `MOD-ReEcho.md`、`MOD-ReEchoUI.md` 与 `Design/UI/ReEcho_UI修改指导.md` 中的小地图笔触职责、资产路径、性能边界和调参入口。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：已审阅；Runtime Module 拓扑与依赖方向不变，无需修改。
  - `README.md`：已审阅；模块/领域索引不变，无需修改。
  - `MOD-ReEcho.md`：已补充小地图笔触属于只读、有界表现投影。
  - `MOD-ReEchoUI.md`：已补充笔触资产、Cook 依赖、回退及调参职责。
  - `Design/UI/ReEcho_UI修改指导.md`：已补充美术交付路径、运行时资产和 WBP 参数说明。

## 锁定验收

- [x] 小地图回响轨迹不再使用生产路径的纯色 `MakeLines`，而是使用交付 512px 笔尖与 Grain UI 材质连续绘制；材质缺失时旧折线 fallback 可用。
- [x] 盖印跨折线段保持近似等距，方向跟随路径，并按确定性种子应用可调角度/透明度变化；相同输入不会逐帧闪烁。
- [x] 保留每个 Echo 的既有轨迹颜色、Player/Echo 头像、透明底板和世界到小地图坐标投影。
- [x] 笔触尺寸、间距、角度变化、不透明度变化与 Grain 强度可在 `WBP_ReEchoEncounterHud` 的小地图控件 Details 调整，非法值被安全钳制。
- [x] 单条轨迹盖印数量有明确上限；零长度段、单点路径和极密路径不会崩溃或产生 NaN。
- [x] 512px 笔尖、Grain 和 UI 材质均可加载并被 Cook 依赖追踪；完整 Photoshop 交付包已归档且不把 `.abr` 当运行时资产。
- [ ] `ReEcho.UI.Minimap`、`ReEcho.UI.CombatHud`、UE 5.8 Editor 构建、静态校验及最终 `-FullRebuild` 发布门禁通过。
- [ ] 用户在 `Level00` 手测轨迹连续性、粗细、颗粒感、颜色、转角、同屏多 Echo 密度和头像遮挡，并确认通过。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`feat/minimap-ink-brush-trail@f8c5b6c64207efb4f4a5ce4cc6e3197585b266ac`，准确基于当前 `origin/main`。
- 引擎/构建可用性：项目标准 UE 5.8 安装版已由 Plan116 最终 FullRebuild 验证；本任务执行时重新验证。
- 现有聚焦测试结果：基线 `ReEcho.UI.Minimap.Transform` 覆盖坐标方向与边界 Clamp；Plan116 最终 `ReEcho.UI.CombatHud.Formatting` 已通过。不得把前序结果冒充本任务最终证据。
- 共享契约 / 难合并资源风险：`ReEchoMinimapCanvasWidget.*` 是本任务主要文本热点；不修改 `WBP_ReEchoEncounterHud.uasset` 即可使用 C++ 默认参数，避免不必要的二进制冲突。精选预构建包仅最终门禁刷新，Editor 操作使用 Git-common-dir 锁。
- 基线损坏时的停止条件：交付笔尖 Alpha 不可用、UI Material 无法在 Slate Brush 中消费 Vertex Color/Alpha、盖印成本无法设定有界上限，或必须改变录制路径契约才能获得连续笔触时，停止扩张并报告用户。

## 实现提纲

1. 归档完整 Photoshop 交付包，幂等导入 512px 笔尖和 Grain 为 UI 纹理，并创建使用 Vertex Color、笔尖 Alpha 与 Grain 调制的半透明 UI 材质。
2. 为 Minimap Canvas 增加 Cook 可追踪材质和 WBP 可调笔触参数；把投影后的折线确定性重采样为有界盖印序列，使用旋转 Slate Box 绘制。
3. 保留材质缺失的旧 `MakeLines` fallback；增加采样等距、确定性、零长度/上限和资产加载自动化。
4. 更新 UI/主模块文档与 Plan 执行记录，完成格式、构建、聚焦自动化、静态校验和人工验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 素材静态 | 尺寸/PixelFormat/Alpha 审计；核对 JSON/TXT 与 `.abr` 归档 | 512 笔尖和 900 Grain 为 32-bit ARGB；运行时不加载 `.abr` |
| 纹理/材质 | Editor 导入/authoring 脚本幂等运行；资产加载自动化 | 两张 UI 纹理、UI Domain Translucent 材质、纹理/标量参数均可加载 |
| C++ 格式 | 对修改 `.h/.cpp` 运行仓库 `.clang-format`；`git diff --check` | 无格式/空白错误 |
| 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码 0 |
| Minimap 聚焦 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.Minimap` | 坐标投影、等距采样、确定性、退化段和上限通过 |
| HUD 回归 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.CombatHud` | 材质/纹理加载及现有 HUD 契约通过 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目、资产依赖、UTF-8 不变量通过 |
| 最终发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终组合候选精选 Editor 包与源码指纹刷新并通过 |
| 人工 | `Level00` 观察单/多 Echo 小地图轨迹 | 连续、稳定、颗粒可读；颜色、图标和 HUD 不回归 |

## 执行记录

### 变化

- 已归档完整 Photoshop 交付包，并新增幂等的运行时纹理导入与 UI 材质 authoring 脚本。
- 已将 Minimap 轨迹生产路径改为确定性、有界的笔尖盖印；保留旧 `MakeLines` 作为材质缺失 fallback，并提供 WBP 实例级笔触参数。

### 证据

- 外部交付包包含 `256×256` 与 `512×512` 透明白色笔尖、`900×900` Grain，三张 PNG 均为 `Format32bppArgb`；另含 `.abr`、关键参数 JSON 与 Photoshop 参数说明。
- 参数说明给出方向跟随、约 `18%` 角度变化、约 `16%` 不透明度变化和 Grain 纹理化语义；运行时实现将保留这些可辨识特征，同时按小地图分辨率设置有界盖印间距。
- UE 5.8 纹理导入脚本连续两次通过，两张 Texture2D 均保存为 UI/无 Mip 资源；材质 authoring 脚本首次 `status=created`、复跑 `status=preserved`，UI Domain、Translucent、两项纹理参数与 `GrainStrength` 自检通过。
- `scripts/ue/Build-Editor.cmd -Configuration Development` 成功并刷新当前候选预构建包；首次 UHT 暴露 UE 5.8 不识别 `Units="px"`，移除纯显示元数据后编译通过。
- `ReEcho.UI.Minimap.InkTrailSampling`、`ReEcho.UI.Minimap.Transform` 与 `ReEcho.UI.CombatHud.Formatting` 全部 `Result={Success}`；覆盖等距、跨段、确定性、退化段/硬上限、既有投影和运行时资产加载。
- `python scripts/validate_project.py` 与 `git diff --check` 通过。发布前仍需在获得 main 发布锁并合并最新 `origin/main` 后执行准确最终组合的 `-FullRebuild`。

### 剩余风险

- 220px 小地图上的 Grain 细节可能因缩放过细或 mip/滤波被弱化，需要真实 HUD 人工调参。
- 密集回放路径的盖印数量与视觉连续性存在性能/清晰度权衡，默认值和上限需在多 Echo 场景验收。

### 人工验收结果/请求

- `PendingBeforeClose`：待实现后由用户在 `Level00` 验收小地图墨水轨迹。

### 架构文档审阅结果

- `ARCHITECTURE.md` 与 CODEBASE_MAP `README.md` 已审阅：本任务不新增 Runtime Module、稳定架构标识或依赖边，因此无需改动。
- `MOD-ReEcho.md`、`MOD-ReEchoUI.md` 与 `Design/UI/ReEcho_UI修改指导.md` 已同步笔触的只读表现边界、资产来源、性能上限、回退和调参入口。
