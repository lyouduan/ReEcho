# Plan 117 - 程序 - 狐狸箭头可见性与批量生成命令

## 协调

- Planner 负责人：Codex（当前程序侧 Planner）。
- Executor 负责人：独立 Executor。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Codex`。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：启动批准为 `origin/main@591a248328528072ff7614689bdeb2b0da612090`；最终合入仅补充本 Plan Writes 的 `origin/main@8db43ef1f1c6a59f1421453533413fe359067e63` 后重跑门禁。
- 本地实现方式：一任务一 worktree；Planner 与 Executor 分离。
- 依赖 / 阻塞：依赖 Plan113 已发布的 Fox Windup/Active/Recovery 事件和 `NS_Fox_Rush_arrow` Fixed Bounds 修复。
- Writes:
  - `Source/ReEcho/{Public,Private}/ReEchoGameMode.*`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxComponent.*`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxCatalog.*`（仅根因需要时）
  - `Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`
  - 适用的 GameMode/GM 自动化测试
  - `scripts/ue/audit_fox_dash_vfx.py`（仅补充只读审计）
  - `docs/GM_COMMANDS.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
  - `plans/117-fox-arrow-visibility-and-batch-spawn.md`
- Stable Reads: `NS_Fox_Rush_arrow.uasset`、Fox Presentation/Profile/Gameplay Blueprint、EnemyLogic 特殊动作事件、生产 `M_FOX` Definition 与 Arena/Roster 生成路径。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：保留 `GMSpawnFox` 单参数旧用法的合理兼容；不改变狐狸技能数值、伤害、冲刺时序、存档或 Niagara 对玩法的只读边界。
- 明确排除：不替换 `NS_Fox_Rush_arrow`，不在 Editor 外改 `.uasset`，不修改狐狸 Profile/Blueprint、XLSX/CSV、其他怪物生成和正式 Encounter 数量。

## 锁定目标

1. 找到 `NS_Fox_Rush_arrow` 已被正确映射和请求生成但画面始终不可见的实际运行时原因，并在 Windup 全程显示沿锁定冲刺方向的箭头；提交/取消/死亡后无残留。
2. Development GM 命令支持 `GMSpawnFox <count> [distance]`，一次生成 X 只生产配置狐狸；数量和距离有安全上限，生成失败具名报告成功/失败数。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`AREA-Presentation`、`AREA-Enemies`、`AREA-Tests`；逻辑模块 `MOD-ReEchoEnemies` 仅作稳定事件读取，不改变。
- 对应模块文档：维护 `MOD-ReEcho.md` 的 GM 命令表面和 `MOD-ReEchoVFX.md` 的 FoxDirection 可见性契约，均已加入 Writes。
- 设计意图：VFX Component 继续只消费权威 Windup 事件并拥有组件生命周期；GM 命令复用 `SpawnConfiguredEnemy`，不得另建测试狐狸类或绕过生产 Definition/Host/Roster。
- 权威状态与依赖：不新增玩法状态；锁定方向由 EnemyLogic/Presentation 事件提供，GameMode 只负责开发期批量调用生产生成入口。
- 决策记录：资产路径正确与静态 Bounds 通过不等于运行时可见；必须检查实际 Niagara 组件激活、粒子实例/材质、组件 Transform、相机平面朝向与生命周期，按第一处失败修复。批量位置采用围绕玩家/朝场地中心的确定性分布并钳制到 Arena，避免 X 只重叠在同一点。
- 相关文档同步范围：审阅 `ARCHITECTURE.md`、`README.md`、`MOD-ReEchoEnemies.md`；预计拓扑、索引和 Enemies 公共契约不变。更新 `docs/GM_COMMANDS.md`、`MOD-ReEcho.md`、`MOD-ReEchoVFX.md`。

## 锁定验收

- [ ] Windup 事件后 Direction 组件有效、激活且具有可渲染粒子/材质证据；PIE 中箭头可见并指向实际冲刺方向。
- [ ] Charging 仍显示；进入 Committed 后 Direction 清理、Trail 显示；取消/死亡/清场无残留。
- [ ] `GMSpawnFox 5 350` 生成 5 只互不完全重叠的生产 `M_FOX`；无参数/旧单参数调用保持安全默认，非法数量被钳制并输出结果。
- [x] FullRebuild、相关自动化、项目校验、prebuilt check、资产只读审计和 `git diff --check` 通过。
- [x] 未提交精选预构建包之外的 UE 生成物或机器路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@591a248328528072ff7614689bdeb2b0da612090`。
- 引擎/构建可用性：Plan113 最终 98-action Development FullRebuild 已通过；本 Plan 不复用其最终证据。
- 现有聚焦测试结果：Plan113 的 Logic、Host、Combat、VFX Catalog 通过，但未覆盖运行时粒子实际可见性。
- 共享契约 / 难合并资源风险：`ReEchoGameMode.*` 与主模块预构建包为共享表面；`NS_Fox_Rush_arrow.uasset` 只读，除非根因证明必须由 Editor 具名修改并先回报 Planner。
- 基线损坏时的停止条件：Windup 没有发布、资产无法加载/编译、所需修复必须改狐狸 Profile/Blueprint/生产数据，或出现产品级生成布局选择。

## 实现提纲

1. 在 Development 路径记录/测试 Direction 的 Spawn 返回值、激活状态、Transform、System 实例及粒子/Renderer 可见条件，区分事件、组件、资产和相机问题。
2. 在 VFX 只读表现边界内修复第一处真实失败，并补能在自动化中锁定根因的断言。
3. 将 `GMSpawnFox` 扩展为 Count + Distance，确定性分散位置、复用 Arena 钳制与 `SpawnConfiguredEnemy`，更新帮助和测试。
4. 维护模块文档与执行记录，执行最终门禁。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| VFX 静态/资产 | VFX Catalog 自动化 + `audit_fox_dash_vfx.py` | 路径、Emitter/Renderer/材质/Bounds/Transform 契约通过 |
| GM/运行时 | 聚焦 GameMode/Enemy Host 自动化 | Count、默认、上限、分散位置和生产生成入口通过 |
| C++ | `.clang-format` + `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新精选包 |
| 项目 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 项目边界、指纹和文本检查通过 |
| 人工 | PIE：`GMSpawnFox 5 350` | 5 只狐狸依次蓄力时箭头可见、方向正确，冲刺 Trail 连续且无残留 |

## 执行记录

### 变化

- `FoxDirection` 继续沿用原 Niagara 资产和 Windup 事件链，仅对新建的 Direction 组件实例覆盖局部 Fixed Bounds 为 `X/Y=[-500,500]、Z=[-650,350]`；未修改 `.uasset`、Profile、Blueprint 或玩法事件。
- 新增 `ReEcho.Presentation.VFX.FoxDirectionRuntime`，通过真实 EnemyEvents → Combat Presentation → CombatVfx 链检查组件、系统实例、粒子、Renderer/材质、运行时 Bounds 和 Committed 清理。
- 第三次本地人工返工按“箭头起始点以狐狸中心为准”收敛：FoxDirection 单独附着生产狐狸 Owner RootComponent，placement 保持零偏移，使组件原点精确等于 Actor/碰撞中心；不再从 Niagara 粒子内部位置猜测并添加 `+150/+300 cm` 补偿。Charging、FoxDash Trail、其 placement/lifecycle/资产保持 `0b339df4` 原样。
- 用户进一步确认修改资产内部布局后，通过受支持的 Niagara Rapid Iteration API 将 `Kuang`、`Kuang002` 的 `InitializeParticle.Position Offset` 从 `(0,0,-150)` 精确改为 `(0,0,0)` 并由 Editor 保存 `NS_Fox_Rush_arrow`；`InitializeParticle.Position` 自身原本已是零。Direction 组件继续零偏移附着 Owner RootComponent，Charging 与 FoxDash Trail 不变。
- 用户 PIE 权威视觉复查确认中心正确，但在实际攻击方向向左时绿色箭头尖端严格向右，证明先前从纹理图像方向推导局部 `+Y` 忽略了 FaceCamera billboard 的屏幕/局部手性。FoxDirection 的真实 authored visual forward axis 修正为组件局部 `-Y`；Catalog 只修正该语义的 authored-axis 映射，继续由既有 `ResolveRotation` 对齐 `Event.LockedDirection`，不改变玩法方向。
- 后续根因复查又证明上述 `-Y` 仍是错误的组件旋转补丁：两个 Sprite renderer 为 `FaceCamera + Automatic/Unaligned`，vertex factory 使用相机 Right/Up 构造 billboard，根本不消费组件旋转来决定纹理尖端。最终修复不再声明 FoxDirection 的 component-local authored axis，并将 Catalog 的 Fox 特例恢复到 `0b339df4` 行为；资产两个启用 Sprite renderer 改为共同绑定 float `User.DirectionSpriteRotationDegrees`，运行时在激活前按实际相机屏幕基写入锁定方向角。UE5.8 `NiagaraSpriteRendererProperties.h` 明确绑定输入为 degrees，`NiagaraSpriteVertexFactory.ush` 再执行 `(value / 180) * PI`，因此未采用会产生单位错误的 `Radians` 参数名。
- 用户随后 PIE 确认尖端方向正确，但 180 度反转时视觉中心上下等量偏移。Editor API 读回两个启用 Sprite renderer 原 `PivotInUVSpace=(0,0.370447)`、`PivotOffsetBinding` 均无有效 source；Editor API 导出的 512x512 源纹理中，`0817_04` 的 alpha>0 bounds 为 `(14,169)-(484,331)`、Y 中心为 `250/511=0.4892367906`，`0817_05` 的 bounds 为 `(87,183)-(429,328)`、Y 中心为 `255.5/511=0.5`。按用户“右降一半/左升一半”将每层 pivot 分别取 `original + 0.5 * (alphaBoundsCenter - original)`，得到 `Kuang=(0,0.4298418953)`、`Kuang002=(0,0.4352235)`；只改 sprite-local Y pivot，X 前向锚点、OwnerRoot/world Z、RotationDegrees、粒子 Local Z、Charging/Trail/伤害均保持不变。
- `GMSpawnFox` 扩展为 `<count> [distance]`，数量钳制 `1..16`、距离钳制 `150..1000 cm`，按朝 Arena 中心的确定性 140 度弧线分散并逐只复用生产 `SpawnConfiguredEnemy("M_FOX")`；无参数与旧单个大距离参数兼容。
- 帮助文本、`docs/GM_COMMANDS.md`、GameMode/VFX 模块文档和聚焦 GM 自动化同步更新。

### 证据

- 基线 `ReEcho.Presentation.VFX.Catalog` 与 `scripts/ue/audit_fox_dash_vfx.py` 通过，只能证明路径、Local Space、Renderer 和资产 Fixed Bounds 静态契约。
- `ReEcho.Presentation.VFX.FoxDirectionRuntime` 的修复前真实运行输出证明 Direction 组件 Registered/Visible/Active，`Kuang`、`Kuang002` 均为 CPU Active 且各有 1 粒子，启用 SpriteRenderer/材质有效；粒子局部位置为 `(0,0,-150)`，初始 SpriteSize 约 `800x600`，而资产 Fixed Bounds 仅 `[-100,100]^3`，首个实际失败为有效粒子被资产包围盒裁剪。
- 合入最终 `origin/main@8db43ef1` 后，Development FullRebuild 以 94 actions 成功并刷新 7 个模块的精选包，Build ID `55116800`、源码指纹 `e60d2f9ea633`。
- 单一正式候选的最终二进制下 `ReEcho.Presentation.VFX` 3/3、`ReEcho.GameMode.GMSpawnFox` 1/1、`ReEcho.Enemies.Host.FoxDashCollision` 1/1 通过；日志分别为 `ReEcho-session-20260826-123757-pid29740.log`、`ReEcho-session-20260826-123825-pid43244.log`、`ReEcho-session-20260826-123846-pid46296.log`。
- 最终只读资产审计输出 `FOX_DASH_AUDIT_OK roots=3 dependencies=12`；`validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过。
- 第三次仅供本地 PIE 的候选以 9-action Development 增量构建成功并刷新精选包，Build ID `55116800`、源码指纹 `8b9d0299bae7`；按用户要求未执行发布级 FullRebuild。
- 同一增量二进制下 `ReEcho.Presentation.VFX` 3/3 通过（`ReEcho-session-20260826-152855-pid19912.log`）。生产 `M_FOX` 运行读回为 `GameplayPlane/AttackVfxRoot/Charging Z=725`，`Actor/OwnerRoot/Direction component Z=855`，Direction attach parent 精确为 Owner RootComponent 且 relative location 为零；粒子只作观察，两个实例均为 `local Z=-150 → world Z=705`，未据此改变组件起点契约。
- 第三次候选下 `ReEcho.Enemies.Host.FoxDashCollision` 1/1 通过（`ReEcho-session-20260826-152950-pid30608.log`）；相对 `0b339df4` 的 Source/Config/Content/scripts patch 不含 `FoxDash`、`Trail` 或 `NS_Fox_Rush_Trail` token，Catalog、Charging/Trail 资产、狐狸 BP/Profile 与只读审计脚本均无差异。
- 本次资产内部布局修复由 Editor API 完成：`ReEcho-session-20260826-160349-pid21900.log` 记录 `Kuang`、`Kuang002` 的 `InitializeParticle.Position Offset` 均由 `(0,0,-150)` 改为 `(0,0,0)` 并保存；独立正式读回 `ReEcho-session-20260826-161018-pid45492.log` 同时确认两个 emitter 的 `Position` 与 `Position Offset` 全部为零且资产可加载。
- 移除临时 authoring seam 后，最终本地候选以 11-action Development 增量构建成功，精选包刷新为 Build ID `55116800`、源码指纹 `2b865291b92a`；未执行发布级 FullRebuild。
- 最终增量二进制下 `ReEcho.Presentation.VFX` 3/3 通过（`ReEcho-session-20260826-161332-pid24648.log`）：生产 `M_FOX` 仍为 `GameplayPlane/AttackVfxRoot/Charging Z=725`、`Actor/OwnerRoot/Direction component Z=855`，两个 emitter 各有 1 个粒子且均为 `local Z=0 → world Z=855`。`ReEcho.Enemies.Host.FoxDashCollision` 1/1 通过（`ReEcho-session-20260826-161427-pid33264.log`）。
- `validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过；相对 `0b339df4` 的 Source/Config/Content/scripts patch 仍无 `FoxDash`、`Trail` 或 `NS_Fox_Rush_Trail` token，Charging/Trail 资产与狐狸生产数据无差异。本地候选未推送，仍等待 PIE 视觉验收。
- 第一轮朝向返工的静态证据确认 renderer 为 `FaceCamera(0) + Automatic(3)`、粒子无额外 `SpriteRotation`，但把向右纹理直接解释为组件局部 `+Y`。对应自动化只证明代码自洽：它用同一个错误轴同时计算组件旋转和验证结果；用户 PIE 中“攻击向左、尖端向右”的严格 180 度反向推翻了该假设，并把 FaceCamera 下的真实视觉轴锁定为组件局部 `-Y`。
- 被 PIE 否决的 `+Y` 候选曾以 6-action Development 增量构建通过并得到源码指纹 `c7bafaf83d19`，VFX 与 Collision 自动化也通过；这些结果只保留为自动化盲区证据，不再作为方向正确性的验收依据。修正后的测试继续验证 `ResolveRotation(...).RotateVector(ResolveAuthoredForwardAxis(FoxDirection)) == LockedDirection`，并在真实 Direction component 上验证世界视觉轴等于 Windup `LockedDirection`，但最终方向仍以二次 PIE 为准。
- PIE 权威证据修正后的 `-Y` 候选以 5-action Development 增量构建成功，精选包刷新为 Build ID `55116800`、源码指纹 `0c54b8fe2ba8`。`ReEcho.Presentation.VFX` 3/3 通过（`ReEcho-session-20260826-164950-pid28024.log`）：真实运行读回 authored visual `-Y` 经组件世界旋转得到 `(0.6,0.8,0)`，与 Windup `LockedDirection=(0.6,0.8,0)` 一致；中心坐标及两个粒子的 `local Z=0 → world Z=855` 保持不变。`ReEcho.Enemies.Host.FoxDashCollision` 1/1 通过（`ReEcho-session-20260826-165047-pid29720.log`）。
- FaceCamera 根因候选通过受支持 Editor API 保存 Direction 资产，`ReEcho-session-20260826-171206-pid33436.log` 记录 `PLAN117_DIRECTION_SPRITE_ROTATION_AUTHORED parameter=User.DirectionSpriteRotationDegrees`；Direction 当前 blob 为 `8d7b8050ed4afe7ef3f4730d1f2e50d7c9337863`。资产 Catalog 已读回精确 float exposed parameter，并确认两个启用 Sprite renderer 的 `SpriteRotationBinding` 都指向它。通用 `Run-Automation.cmd` 固定使用 `-nullrhi`，该模式下引擎 `FApp::CanEverRender=0`、Niagara 工厂按设计不创建组件，故运行时 VFX 证据必须使用 `-RenderOffscreen`；此环境差异不作为产品失败。
- 远端同路径适配风险：实现期间只读复查的最新 `origin/main@69539c79cebf55801939a2d8283b5d8b21717c0f` 相对 `0b339df4` 已修改 `ReEchoCombatVfxComponent.{h,cpp}` 与 `ReEchoCombatVfxTests.cpp`，但未修改 Direction 资产。当前仅保留本地候选，不合并、不推送；未来进入 main 前必须按实际最新远端逐 hunk 适配这些共享源码。
- 最终本地候选以 9-action Development 增量构建成功，精选包刷新为 Build ID `55116800`、源码指纹 `5a0c80e79d3b`；按用户要求未执行发布级 FullRebuild。`-RenderOffscreen` 下 `ReEcho.Presentation.VFX` 3/3 通过（`ReEcho-session-20260826-173607-pid22216.log`）：真实 `PlayerCameraManager` 使用 `Pitch=-60/Yaw=25` 的倾斜 CameraCache，四个 `LockedDirection` 的组件 override 分别为 `-118.300°/-21.991°/61.700°/-58.312°`，逐项等于 renderer shader 的相机屏幕基期望值；生产 M_FOX 仍读回 `GameplayPlane/AttackVfxRoot/Charging Z=725`、`Actor/OwnerRoot/Direction/两 emitter 粒子 Z=855`。`ReEcho.Enemies.Host.FoxDashCollision` 1/1 通过（`ReEcho-session-20260826-173733-pid46092.log`）；只读资产审计通过（`ReEcho-session-20260826-173833-pid46908.log`，`FOX_DASH_AUDIT_OK roots=3 dependencies=12 native_contract=ReEcho.Presentation.VFX.Catalog`）。`validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过；Charging/Trail blob 分别为 `085f24ba12bbe27f3db4fb135750fe2628bbf60e`、`3ea66280d5447d26f5e5e229e858b40d971b56b7`，与 `0b339df4` 完全一致。
- Pivot 修复通过持有同克隆 Unreal 锁的受支持 Editor seam 保存 Direction 资产；`ReEcho-session-20260826-180634-pid33664.log` 精确读回 `Kuang=(0,0.429841895)`、`Kuang002=(0,0.435223500)` 且二者 `PivotOffsetBinding` 均无有效 source。源码契约测试按上述两张纹理各自的 alpha bounds 中心重算目标值，避免把两层强制成同一 pivot。
- 当前 pivot 候选以 9-action Development 增量构建成功，精选包刷新为 Build ID `55116800`、源码指纹 `5172a3d61e2b`；未执行发布级 FullRebuild。`-RenderOffscreen` 下 `ReEcho.Presentation.VFX` 3/3 通过（`ReEcho-session-20260826-180850-pid18508.log`），Catalog 与真实 Runtime 同时读回上述两个独立 pivot，四方向 RotationDegrees、OwnerRoot 中心、Local Z=0 与 Charging 地面挂点契约均保持通过。`ReEcho.Enemies.Host.FoxDashCollision` 1/1 通过（`ReEcho-session-20260826-180943-pid41960.log`）；只读资产审计通过（`ReEcho-session-20260826-181026-pid44928.log`，`FOX_DASH_AUDIT_OK roots=3 dependencies=12 native_contract=ReEcho.Presentation.VFX.Catalog`）。`validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过。Direction 当前 blob 为 `f8fd6f50015596d89f5c4520bbc53b4be0854ceb`；Charging/Trail blob 仍分别为 `085f24ba12bbe27f3db4fb135750fe2628bbf60e`、`3ea66280d5447d26f5e5e229e858b40d971b56b7`，与 `0b339df4` 完全一致；本次 Source/Config/Content/scripts diff 不含 Trail 行为修改。

### 剩余风险

- 自动化可锁定真实事件链、粒子/Renderer 与裁剪范围，但 `-RenderOffscreen` 不等价于玩家相机下的最终画面；箭头朝向、尺寸和前景遮挡仍需 PIE 人工确认。
- 旧单参数 `1..16` 现在按数量解释；旧距离命令通常远大于 16 并保持兼容，若团队曾用 `GMSpawnFox 10` 表示距离，它将改为生成 10 只。

### 人工验收结果/请求

- `PendingBeforeClose`：PIE 执行 `GMSpawnFox 5 350`，确认 5 只生产狐狸互不完全重叠；观察每只 Windup/Charging 箭头可见且指向实际冲刺方向，Committed 后箭头清理并显示连续 Trail，取消/死亡/清场无残留。

### 架构文档审阅结果

- 已更新 `docs/GM_COMMANDS.md`、`MOD-ReEcho.md` 的 GM 命令表面和 `MOD-ReEchoVFX.md` 的 Direction 运行时 Bounds/自动化契约。
- 已审阅根 `README.md`、`shared/CODEBASE_MAP/ARCHITECTURE.md`、`shared/CODEBASE_MAP/README.md` 与 `MOD-ReEchoEnemies.md`；Runtime Module 拓扑、索引和 Enemies 公共事件契约未改变，无需更新。
