# ReEcho Planner 交换区

仅记录实时本地协调。已关闭历史归入 Plan 和 Git；永久规则归入权威规则文件。及时移除已释放所有权和过期引用。

## 已规划和活跃工作公告

| Plan | Plan AI / 实现 AI | Planner | 任务状态 | 人工验收 | 引用 / 基线 | 影响与 Writes | 依赖 / 其他 Planner 操作 |
|---|---|---|---|---|---|---|---|
| Plan 29 完整 UI UMG 迁移 | ReEcho teammate-side / ReEcho teammate-side | Codex | `Review` | `PendingBeforeClose` | 已通过 Plan31 候选集成到本地 `main` | UMG 资产和类型化 UI Framework 拥有屏幕生命周期；玩法/数据契约保持分离 | 保留 UI Flow 所有权，不得恢复直接 viewport 或代码构建页面生命周期。 |
| Plan 10 玩家 HUD UMG 跟进 | ReEcho teammate-side / ReEcho teammate-side | Codex | `Review` | `PendingBeforeClose` | 已验收实现谱系包含在 Plan29 中 | 隔离 C++ 和 `/Game/ReEcho/UI/WBP_ReEchoPlayerHud.uasset` | 关闭前需要人工 PIE 完成视觉/DPI 验收。 |
| Plan 26 武器只读 UI 消费者 | Teammate-side / teammate-side | ReEcho teammate-side Planner | `Ready` | `PendingBeforeClose` | 从 Planner 批准的当前 main/代码基线开始 | 只读 provider 消费加隔离 Inventory/Stats 消费者工作 | 保留 Plan29 表现绑定，不编辑数据 provider 契约。 |
| Plan 28 自动/手动玩家攻击 | Gavyn-side / Gavyn-side | Gavyn-side Planner | `Review` | `PendingBeforeClose` | 已通过合并 `a931af3` 交付到 `main` | 玩家攻击模式、确定性目标选择、v6 持久化、按模式门控物理输入、P 暂停和 Restart 子面板 | 输入源缺陷已修正。静态/构建/Blueprint 门禁通过；聚焦 5/5、完整 ReEcho 自动化 51/51 通过。仍需人工 PIE/手感验收。 |
| Plan 34 编写音频目录和持久设置 | Gavyn-side / Gavyn-side | Gavyn-side Planner | `Ready` | `PendingBeforeClose` | 本地 `plan/34-audio-catalog-settings@86824a0`，基于 `f5051e4`；WIP 保留等待 Plan41 后适配 | AudioEvents XLSX/CSV 契约、异步目录/预加载、五条持久总线和设置控件 | 为 Plan41 暂停 Active 所有权；绝不整块合并旧工作簿 blob，之后在新增 Combat/Weapons 后的四模块 main 上适配。 |
| Plans 35-36 非战斗与战斗/Echo 音频集成 | Gavyn-side / `Unassigned` | Gavyn-side Planner | `Proposed` | `PendingBeforeClose` | Plan33 API 已可用；可听关闭依赖 Plan34 目录/资产 | 只接入语义事件；音频模块保留播放所有权 | 可使用稳定 Plan33 ID，但不得发明目录解析、资产路径或并行音频服务。 |
| Plan 40 数据驱动 2D 角色表现与逐帧碰撞 | ReEcho teammate-side / ReEcho teammate-side | Codex | `Review` | `PendingBeforeClose` | 本地 `main@4941c19` 之后的资产迁移候选；已合入远端架构权威 | 共享动画/profile/controller/碰撞轨道契约，独占 `/Game/ReEcho/Art/Animation2D/**` 与 `/Game/ReEcho/Animation2D/**` | 客观发布门禁进行中；保留攻击重试语义和 Capsule 移动权威，逐帧 Paper2D 轮廓仅 Query。 |
| Plan 41 Combat/Weapons 逻辑模块与攻击系统重构 | Gavyn-side / Gavyn-side | Gavyn-side Planner | `InProgress` | `PendingBeforeClose` | 已审计合入 `origin/main@24b9f08`；本地 `plan/41-combat-runtime-module` | 新 `ReEchoCombat` + `ReEchoWeapons`、统一 AttackIdentity、HitIntent/HitResolved、逻辑攻击载体、只读表现 Events/Snapshots、GAS/Combatant 与反射兼容 | 用户审核扩展为 Combat 最终裁决、Weapons 生成攻击、表现只读消费；旧单模块证据失效，实施和客观验证继续。 |
| Plan 42 Editor 可编辑的第一关竞技场 | ReEcho teammate-side / `Unassigned` | Codex | `Ready` | `PendingBeforeClose` | `origin/main@52084b1`；本地未跟踪 `Content/ReEcho/Art/Scene/map01.*` 作为隔离输入 | 新 Arena Scene Actor、Level00、map01、玩家跟随/地图边缘锁定相机、GameMode/Player 窄适配、Arena 测试与 `MOD-ReEcho.md` | 相机视野、背景与玩法边界拆分；视锥 footprint 限制画面不露出地图外；实施前隔离脏美术。 |

## 活跃所有权

| 所有者 | Plan/分支 | 模式 | 状态 | 文件或独占资源 | 备注 |
|---|---|---|---|---|---|
| ReEcho teammate-side Planner | Plan26 / teammate-side 任务分支 | `ReadOnly` | `Reserved` | 类型化武器 UI 消费行为和 Plan26 测试 | 必须保留 Plan29 表现绑定。 |
| Gavyn-side Planner | Plan34 / `plan/34-audio-catalog-settings` | `SharedContract` + `Exclusive` | `Reserved` | `Design/Data/ReEchoData.xlsx`、生成的 `Content/Data/audio_events.csv`、设置 UI 和 Plan33 目录/设置 provider 表面 | WIP 完整保留；Plan41 结束前暂停写入 Build.cs/预构建包，之后适配新模块 main。 |
| Codex | Plan40 / local `main` candidate | `SharedContract` + `Exclusive` | `Active` | `Presentation/Animation2D/**`、最小必需玩家/敌人表现调用点、`/Game/ReEcho/Art/Animation2D/**`、`/Game/ReEcho/Animation2D/**`、动画/碰撞测试和工具 | Review/发布门禁中。逐帧轮廓仅 Query；不得替换稳定移动 Capsule，人工 PIE 未完成前不关闭。 |
| Gavyn-side Planner | Plan41 / `plan/41-combat-runtime-module` | `SharedContract` + `Exclusive` | `Active` | `ReEchoCombat`、`ReEchoWeapons`、AttackController/Targeting/WeaponRuntime/HitResolver/CombatEvents、战斗/GAS/武器公共类型、模块描述符、Core Redirects、最终预构建包 | 双逻辑模块实施中；只提供 UI、动画、VFX、音频的只读适配契约，不认领其资源、音频目录或权威 XLSX。 |
| Codex | Plan42 / `plan/42-editor-authored-first-arena` | `SharedContract` + `Exclusive` | `Reserved` | `Content/Level00.umap`、`Content/ReEcho/Art/Scene/map01.*`、Arena Scene Actor、GameMode/Player 场景接缝、Arena 测试与 `MOD-ReEcho.md` | Executor 启动后才变为 Active；map02..04 不在本 Plan，现有脏美术必须先隔离。 |

## 警告 / 阻塞项

- 编号或远端集成前 fetch `origin/main`。若其前进，在 pull/merge/rebase/push 前报告物理、逻辑、耦合和编号差异。
- 禁止远端侧分支；Plan、任务分支和 worktree 在已验收范围进入 main 前保持本地。
- 旧远端 `origin/review/gavyn-plan28-attack-modes-20260812` 仍有五个与当前 `main` 不具 patch 等价性的提交。不得合并；先比较其语义与已关闭 Plan28/38 结果，再决定该引用是否可安全删除。
- `Design/Data/ReEchoData.xlsx` 仅在存在准备发布的编辑时是单写入者二进制。一次性本地可用性测试不锁定团队资源。
- 运行时竞技场没有专用序列化测试地图；替换任何 `.umap` 前先认领。
- Plan34 不可整分支合并：该分支与当前 main 都修改了 `Design/Data/ReEchoData.xlsx`、`Source/ReEcho/ReEcho.Build.cs`、`scripts/data/sync_xlsx_to_csv.py` 和 `scripts/validate_project.py`。在当前权威工作簿上重新生成 AudioEvents Table，明确组合文本契约，并重跑全部数据/构建证据。
- Plan41 与 Plan34 在 `Source/ReEcho/ReEcho.Build.cs` 和最终 Editor 预构建包发生真实写入重叠；与 Plan40 在 `PlayerPawn`/敌人调用点和攻击/表现公共契约发生逻辑耦合。当前明确顺序为 Plan41 先执行，Plan34/40 保持 Reserved 并在新模块边界上适配。
- Plan41 锁定 `weapons.AttackIntervalSeconds / AttackSpeed` 为自动/手动普通攻击唯一频率；`attack_steps.DurationSeconds` 只描述非阻塞步骤行为，不得成为第二道攻击门。长剑等当前被步骤时长压慢的武器会按武器体系表提速，等待用户 PIE 验收。
- Plan41 的 UI/Audio/Presentation 接缝固定为“类型化 Event Payload + 只读 Snapshot/provider + 受控 Command”；主模块负责适配，Combat/Weapons 与 Audio 不直接互相依赖，Widget 不得写逻辑模块内部状态。
- Plan41 新边界：`ReEchoWeapons` 独占武器节奏、步骤、近战和逻辑 Projectile/Wave，只产生 HitIntent；`ReEchoCombat` 独占伤害/元素/生命/死亡最终裁决。所有表现只读事件/快照，资源或动画状态不得反向控制逻辑。
- 已锁定产品决定：选择攻击模式不会自动恢复；Continue 恢复本局选择，新局默认自动；每个活跃 Echo 都提供迷雾视野；显式回放选择为空时不生成 Echo。

## 待人工决定

- 无。
