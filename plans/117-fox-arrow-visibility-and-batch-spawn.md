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
- `GMSpawnFox` 扩展为 `<count> [distance]`，数量钳制 `1..16`、距离钳制 `150..1000 cm`，按朝 Arena 中心的确定性 140 度弧线分散并逐只复用生产 `SpawnConfiguredEnemy("M_FOX")`；无参数与旧单个大距离参数兼容。
- 帮助文本、`docs/GM_COMMANDS.md`、GameMode/VFX 模块文档和聚焦 GM 自动化同步更新。

### 证据

- 基线 `ReEcho.Presentation.VFX.Catalog` 与 `scripts/ue/audit_fox_dash_vfx.py` 通过，只能证明路径、Local Space、Renderer 和资产 Fixed Bounds 静态契约。
- `ReEcho.Presentation.VFX.FoxDirectionRuntime` 的修复前真实运行输出证明 Direction 组件 Registered/Visible/Active，`Kuang`、`Kuang002` 均为 CPU Active 且各有 1 粒子，启用 SpriteRenderer/材质有效；粒子局部位置为 `(0,0,-150)`，初始 SpriteSize 约 `800x600`，而资产 Fixed Bounds 仅 `[-100,100]^3`，首个实际失败为有效粒子被资产包围盒裁剪。
- 合入最终 `origin/main@8db43ef1` 后，Development FullRebuild 以 94 actions 成功并刷新 7 个模块的精选包，Build ID `55116800`、源码指纹 `e60d2f9ea633`。
- 单一正式候选的最终二进制下 `ReEcho.Presentation.VFX` 3/3、`ReEcho.GameMode.GMSpawnFox` 1/1、`ReEcho.Enemies.Host.FoxDashCollision` 1/1 通过；日志分别为 `ReEcho-session-20260826-123757-pid29740.log`、`ReEcho-session-20260826-123825-pid43244.log`、`ReEcho-session-20260826-123846-pid46296.log`。
- 最终只读资产审计输出 `FOX_DASH_AUDIT_OK roots=3 dependencies=12`；`validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过。

### 剩余风险

- 自动化可锁定真实事件链、粒子/Renderer 与裁剪范围，但 `-RenderOffscreen` 不等价于玩家相机下的最终画面；箭头朝向、尺寸和前景遮挡仍需 PIE 人工确认。
- 旧单参数 `1..16` 现在按数量解释；旧距离命令通常远大于 16 并保持兼容，若团队曾用 `GMSpawnFox 10` 表示距离，它将改为生成 10 只。

### 人工验收结果/请求

- `PendingBeforeClose`：PIE 执行 `GMSpawnFox 5 350`，确认 5 只生产狐狸互不完全重叠；观察每只 Windup/Charging 箭头可见且指向实际冲刺方向，Committed 后箭头清理并显示连续 Trail，取消/死亡/清场无残留。

### 架构文档审阅结果

- 已更新 `docs/GM_COMMANDS.md`、`MOD-ReEcho.md` 的 GM 命令表面和 `MOD-ReEchoVFX.md` 的 Direction 运行时 Bounds/自动化契约。
- 已审阅根 `README.md`、`shared/CODEBASE_MAP/ARCHITECTURE.md`、`shared/CODEBASE_MAP/README.md` 与 `MOD-ReEchoEnemies.md`；Runtime Module 拓扑、索引和 Enemies 公共事件契约未改变，无需更新。
