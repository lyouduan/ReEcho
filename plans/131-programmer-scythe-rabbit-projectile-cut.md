# Plan 131 - 程序 - 镰刀斩断兔子子弹

## 协调

- Planner 负责人：Codex（程序路线）。
- Executor 负责人：Codex（程序路线）。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Closed`。
- 人工验收：`NotRequired`。
- 本地规划 / 实现基线：`origin/main@1812dfb1d2f73ec9cb38137812196e7754e3bf53`。
- 本地实现方式（可选，仅作交接说明）：`C:\tmp\ReEcho-plan131-scythe-projectile-cut`，分支 `codex/plan131-scythe-projectile-cut`。
- 依赖 / 阻塞：复用已发布长剑斩弹的 EnemyHost 逻辑球查询、`Ended` 事件和视觉代理清理契约。
- Writes: `plans/131-programmer-scythe-rabbit-projectile-cut.md`；`Source/ReEcho/Public/Weapons/ReEchoWeaponActor.h`；`Source/ReEcho/Private/Weapons/ReEchoWeaponActor.cpp`；`Source/ReEcho/Private/Tests/ReEchoWeaponRuntimeTests.cpp`；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`；FullRebuild 刷新的 `Binaries/Win64/ReEchoEditor.prebuilt.json` 及其准确允许列表产物。
- Stable Reads: `Source/ReEcho/Public/Graybox/ReEchoEnemyActor.h`；`Source/ReEcho/Private/Graybox/ReEchoEnemyActor.cpp`；`Source/ReEcho/Private/Tests/ReEchoEnemyHostTests.cpp`；武器 CSV 中长剑/镰刀的 `AttackPatternId`、`RangeCm`、`ArcDegrees`。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：长剑保持前方 180°斩弹；镰刀使用自身提交的 360°弧与范围；弓、枪、非兔子投射物不获得斩弹能力。斩弹不产生额外伤害、攻击次数或独立视觉碰撞。
- 明确排除：不修改特效挂点、武器动画、兔子子弹伤害/速度/半径、非兔子敌方投射物、武器范围与弧度数据。

## 锁定目标

镰刀每次成功提交 `Pattern.ScytheSweep` 近战攻击时，必须与长剑复用同一权威兔子逻辑球斩断路径，并使用该次 Commit 已确定的 Origin、AimDirection、RangeCm 与 360° ArcDegrees；弧内逻辑球发布 `Ended` 并移除，之后不能继续伤害角色，对应视觉代理随事件消失。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` / `AREA-Weapons`、`MOD-ReEchoWeapons`；EnemyHost 契约仅被复用，不修改 `MOD-ReEchoEnemies`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `MOD-ReEchoWeapons.md`，均已加入 `Writes`。
- 设计意图：把“允许斩断兔子子弹的近战 Pattern”集中为显式资格判断，继续让 EnemyHost 的逻辑球和 `Ended` 事件拥有生命周期权威。
- 权威状态与依赖：不改变状态所有者或公共模块依赖；主模块 WeaponActor 在近战 Commit 后调用现有 EnemyHost 窄接口。
- 决策记录：允许 `Pattern.LongSwordCombo` 与 `Pattern.ScytheSweep`；拒绝使用武器贴图、刀光 Bounds 或命中特效做碰撞。镰刀不新增第二次查询，直接使用其现有 360°近战提交参数。
- 相关文档同步范围：维护上述两个 `MOD-*`；审阅 `ARCHITECTURE.md`、`README.md` 与 `MOD-ReEchoEnemies.md`，拓扑、路由和提供者契约未变化时记录无需修改。
- 关闭前逐项填写审阅结果：待实现后补充。

## 锁定验收

- [x] 资格测试证明长剑与镰刀可斩兔子子弹，弓枪及未知 Pattern 不可。
- [x] 镰刀调用现有 `DestroyRabbitProjectilesInMeleeArc`，参数来自本次 Commit 的相同 Origin、AimDirection、RangeCm、ArcDegrees。
- [x] 现有 EnemyHost 自动化继续证明弧内逻辑球发布 `Ended`、移除视觉代理且不能造成后续伤害。
- [x] FullRebuild、聚焦自动化、项目校验、预构建检查和 `git diff --check` 通过。
- [x] 未提交精选 `GIT_RULES.md` 允许列表之外的生成物。

## Step 0 门禁

- 基线分支/提交：`origin/main@1812dfb1d2f73ec9cb38137812196e7754e3bf53`。
- 引擎/构建可用性：使用 UE 5.8 Windows 仓库入口；构建前确认 Editor 已关闭。
- 现有聚焦测试结果：静态确认长剑资格硬编码在 `AReEchoWeaponActor::SwingMelee`；EnemyHost 已覆盖逻辑球删除、`Ended` 与后续无伤害。
- 共享契约 / 难合并资源风险：不修改二进制资产；WeaponActor 可能与 Plan126 本地未发布表现候选重叠，两个独立候选后续集成时必须按函数级组合，不能整分支覆盖。
- 基线损坏时的停止条件：现有兔子逻辑球测试在未修改基线失败、远端改变斩弹所有者，或扩展到非兔子投射物需要新的产品决定。

## 实现提纲

1. 提取可测试的近战 Pattern 斩弹资格判断，加入镰刀 Pattern。
2. 让 `SwingMelee` 使用该判断调用现有 EnemyHost 弧形逻辑球删除接口。
3. 增加长剑、镰刀与负例自动化，回归 EnemyHost 兔子弹生命周期测试。
4. 维护模块文档和执行记录，完成构建与验证。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目与源码不变量通过 |
| C++ | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新精选预构建包 |
| 聚焦自动化 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Weapons.Runtime` | Pattern 资格和 WeaponActor 接缝通过 |
| 生命周期回归 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Enemies.Host.RabbitProjectilePipeline` | 逻辑球 Ended、视觉清理和后续无伤害通过 |

## 执行记录

### 变化

- `AReEchoWeaponActor` 集中判断可斩兔子子弹的近战 Pattern，保留 `Pattern.LongSwordCombo` 并加入 `Pattern.ScytheSweep`。
- `SwingMelee` 对两种 Pattern 复用同一 OwnerLocation、AimDirection、Commit RangeCm/ArcDegrees 和 EnemyHost `DestroyRabbitProjectilesInMeleeArc` 路径。
- 新增资格自动化，覆盖长剑、镰刀、弓、枪与未知 Pattern；模块文档同步镰刀 360°斩弹语义。

### 证据

- `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`：97/97，成功；预构建源码指纹 `394294338bdb`。
- `ReEcho.Weapons.Runtime.MeleeProjectileCutEligibility`：发现 1 项，`Result={Success}`。
- `ReEcho.Enemies.Host.RabbitProjectilePipeline`：发现 1 项，`Result={Success}`，覆盖逻辑球 `Ended`、视觉代理清理及后续无伤害。
- `python scripts\validate_project.py`、`python scripts\ue\prebuilt_editor.py check`、`git diff --check`：通过；预构建 7 模块、Build ID `55116800`。

### 剩余风险

- Plan126 的本地 WeaponActor 表现候选尚未发布；后续发布时必须从包含本 Plan 的最新 main 组合函数级差异并重跑最终门禁，不能整文件覆盖。

### 人工验收结果/请求

`NotRequired`：该行为由确定性逻辑球、资格和生命周期自动化覆盖。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已更新：记录长剑 180°与镰刀 360°复用权威逻辑球结束路径。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md` 已更新：记录两种 Pattern 的资格、Commit 参数来源和排除项。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md` 已审阅、无需修改：EnemyHost 的逻辑球移除、`Ended` 与视觉消费契约未变化。
- `shared/CODEBASE_MAP/ARCHITECTURE.md` 已审阅、无需修改：模块拓扑和依赖方向未变化。
- `shared/CODEBASE_MAP/README.md` 已审阅、无需修改：稳定模块与 AREA 路由未变化。
